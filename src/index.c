#include "index.h"
#include "util.h"
#include "unpack.h"

// TODO: start_data is set in two different places here - for
// everything in index_build() just after setting refid, but also
// within some of the functions.  It's not clear that this is being
// done that badly, but it feels like there's some repetition here.

// NOTE: Using Calloc here because this needs to survive past the R
// call (so R_alloc) is not appropriate; basically treating this is a
// smart pointer so that we can throw safely anywhere (we hit a lot of
// the R API) but not leak.

static void r_index_finalize(SEXP r_ptr);

// this probably mixes up two functions
const rds_index_t * index_build(SEXP r_x) {
  // Do this first, because that's where the argument validation is
  unpack_data_t *obj = unpack_data_create_r(r_x);

  rds_index_mutable_t *index =
    (rds_index_mutable_t*) Calloc(1, rds_index_mutable_t);
  SEXP ptr = PROTECT(R_MakeExternalPtr(index, R_NilValue, R_NilValue));
  R_RegisterCFinalizer(ptr, r_index_finalize);

  // Initialise things
  index->objects = NULL;
  index->len = 100; // initial length
  index_grow(index);
  index->n_refs = 0;
  index->len_data = obj->buffer->len;

  obj->index = NULL;
  obj->ref_objects = PROTECT(init_read_index_ref());

  index_item(obj, index, 0);
  buffer_check_empty(obj->buffer);
  index->len = obj->count;

  obj->ref_objects = R_NilValue;

  rds_index_t *ret = (rds_index_t*) Calloc(1, rds_index_t);
  ret->objects = index->objects;
  ret->len = index->len;
  ret->len_data = index->len_data;
  ret->n_refs = index->n_refs;

  R_ClearExternalPtr(ptr);
  UNPROTECT(2);

  return ret;
}

static void r_index_finalize(SEXP r_ptr) {
  rds_index_mutable_t * index = (rds_index_mutable_t *)
    check_extptr_valid(r_ptr, "ptr", false);
  if (index != NULL) {
    Free(index->objects);
    Free(index);
    R_ClearExternalPtr(r_ptr);
  }
}

void index_grow(rds_index_mutable_t *index) {
  if (index->objects != NULL) {
    index->len *= 2;
    // NOTE: when doing grow, we need to re-lookup all indices that
    // are live.  So index_build could return true/false or we need to
    // be careful when storing pointers from index.
  }
  index->objects =
    (sexp_info_t*) Realloc(index->objects, index->len, sexp_info_t);
}

void index_item(unpack_data_t *obj, rds_index_mutable_t *index, size_t parent) {
  if (obj->count == index->len) {
    index_grow(index);
  }
  size_t id = obj->count;
  sexp_info_t *info = index->objects + id;
  info->start_object = obj->buffer->pos;
  unpack_flags(buffer_read_integer(obj->buffer), info);
  info->parent = parent;
  info->id = id;
  info->refid = 0; // this is a lie but it does not matter
  info->start_data = obj->buffer->pos;
  obj->count++;

  switch (info->type) {
    // 1. Some singletons:
  case NILVALUE_SXP:
  case EMPTYENV_SXP:
  case BASEENV_SXP:
  case GLOBALENV_SXP:
  case UNBOUNDVALUE_SXP:
  case MISSINGARG_SXP:
  case BASENAMESPACE_SXP:
    info->length = info->type != NILVALUE_SXP;
    info->start_attr = obj->buffer->pos;
    info->end = obj->buffer->pos;
    return;
    // 2. Reference objects; none of these are handled (yet? ever?)
  case REFSXP:
    index_ref(obj, index, id);
    return;
  case PERSISTSXP:
    Rf_error("unimplemented: index_persistent");
  case PACKAGESXP:
    index_package(obj, index, id);
    return;
  case NAMESPACESXP:
    index_namespace(obj, index, id);
    return;
  case ENVSXP:
    index_environment(obj, index, id);
    return;
    // 3. Symbols
  case SYMSXP:
    index_symbol(obj, index, id);
    return;
    // 4. Dotted pair lists
  case LISTSXP:
  case LANGSXP:
  case CLOSXP:
  case PROMSXP:
  case DOTSXP:
    index_pairlist(obj, index, id);
    return;
    // 5. References
  case EXTPTRSXP:  // +attr
    index_extptr(obj, index, id);
    return;
  case WEAKREFSXP: // +attr
    index_weakref(obj, index, id);
    return;
    // 6. Special functions
  case SPECIALSXP: // +attr
  case BUILTINSXP: // +attr
    Rf_error("Can't unpack objects of type %s",
             CHAR(type2str(info->type)));
    // 7. Single string
  case CHARSXP: // +attr
    index_charsxp(obj, index, id);
    return;
    // 8. Vectors!
  case LGLSXP: // +attr
  case INTSXP: // +attr
    index_vector(obj, index, id, sizeof(int));
    return;
  case REALSXP: // +attr
    index_vector(obj, index, id, sizeof(double));
    return;
  case CPLXSXP: // +attr
    index_vector(obj, index, id, sizeof(Rcomplex));
    return;
  case STRSXP: // +attr
    index_vector_character(obj, index, id);
    return;
  case VECSXP: // +attr
  case EXPRSXP: // +attr
    index_vector_generic(obj, index, id);
    return;
    // 9. Weird shit
  case BCODESXP: // +attr
  case CLASSREFSXP: // +attr
  case GENERICREFSXP: // +attr
    Rf_error("Can't unpack objects of type %s",
             CHAR(type2str(info->type)));
    // 10. More vectors
  case RAWSXP: // +attr
    index_vector(obj, index, id, sizeof(char));
    return;
    // 11. S4
  case S4SXP: // +attr
    Rf_error("implement this");
    return;
    // 12. Unknown types
  default:
      Rf_error("Can't unpack objects of type %s",
               CHAR(type2str(info->type)));
  }
}

void index_vector(unpack_data_t *obj, rds_index_mutable_t *index, size_t id, size_t element_size) {
  sexp_info_t *info = index->objects + id;
  info->length = buffer_read_length(obj->buffer);
  info->start_data = obj->buffer->pos;
  switch (obj->buffer->format) {
  case BINARY:
  case XDR:
    buffer_advance(obj->buffer, element_size * info->length);
    break;
  case ASCII:
  default:
    Rf_error("not implemented (index_vector)");
  }
  index_attributes(obj, index, id);
}

void index_vector_character(unpack_data_t *obj, rds_index_mutable_t *index, size_t id) {
  index_vector_generic(obj, index, id);
}

void index_vector_generic(unpack_data_t *obj, rds_index_mutable_t *index, size_t id) {
  index->objects[id].length = buffer_read_length(obj->buffer);
  index->objects[id].start_data = obj->buffer->pos;
  for (R_xlen_t i = 0; i < index->objects[id].length; ++i) {
    index_item(obj, index, id);
  }
  index_attributes(obj, index, id);
}

void index_symbol(unpack_data_t *obj, rds_index_mutable_t *index, size_t id) {
  index->objects[id].length = 1;
  index_item(obj, index, id);
  index->objects[id].start_attr = obj->buffer->pos;
  index->objects[id].end = obj->buffer->pos;
  add_read_index_ref(obj, index, id);
}

void index_pairlist(unpack_data_t *obj, rds_index_mutable_t *index, size_t id) {
  index->objects[id].length = 1; // pairlist always have one element
  index->objects[id].start_attr = obj->buffer->pos;
  if (index->objects[id].has_attr) {
    index_item(obj, index, id);
  }
  // We index this but it will not sensibly appear in some ways.  We
  // could infer it from the combination of has_tag and has_attr and
  // the locations of the pointers but it will be quite hard.  So
  // getting the attribute out will be easy but not the cdr.  I'm not
  // really sure that this is rich enough to help us index into
  // pairlists actually.
  if (index->objects[id].has_tag) {
    index_item(obj, index, id);
  }
  index_item(obj, index, id); // car
  index->objects[id].start_data = obj->buffer->pos;
  index_item(obj, index, id); // cdr
  index->objects[id].end = obj->buffer->pos;
}

void index_charsxp(unpack_data_t *obj, rds_index_mutable_t *index, size_t id) {
  sexp_info_t *info = index->objects + id;
  // NOTE: *not* read_length() because limited to 2^32 - 1
  info->length = buffer_read_integer(obj->buffer);
  info->start_data = obj->buffer->pos;
  if (info->length > 0) {
    buffer_advance(obj->buffer, info->length);
  }
  index_attributes(obj, index, id);
}

void index_ref(unpack_data_t *obj, rds_index_mutable_t *index, size_t id) {
  sexp_info_t *info = index->objects + id;
  int idx = unpack_read_ref_index(obj, info);
  info->length = 0;
  info->start_data = obj->buffer->pos;
  info->start_attr = obj->buffer->pos;
  info->end = obj->buffer->pos;
  info->refid = get_read_index_ref(obj, index, idx);
}

void index_package(unpack_data_t *obj, rds_index_mutable_t *index, size_t id) {
  index_persistent_string(obj, index, id);
  add_read_index_ref(obj, index, id);
}

void index_namespace(unpack_data_t *obj, rds_index_mutable_t *index, size_t id) {
  index_persistent_string(obj, index, id);
  add_read_index_ref(obj, index, id);
}

void index_environment(unpack_data_t *obj, rds_index_mutable_t *index, size_t id) {
  index->objects[id].length = 1;
  buffer_read_integer(obj->buffer); // locked/unlocked (TODO: advance_integer instead?)
  add_read_index_ref(obj, index, id);
  index->objects[id].start_data = obj->buffer->pos;
  index_item(obj, index, id); // enclos
  index_item(obj, index, id); // frame
  index_item(obj, index, id); // hashtab
  index->objects[id].start_attr = obj->buffer->pos;
  index_item(obj, index, id); // attrib
  index->objects[id].end = obj->buffer->pos;
}

void index_extptr(unpack_data_t *obj, rds_index_mutable_t *index, size_t id) {
  index->objects[id].length = 1;
  add_read_index_ref(obj, index, id);
  index_item(obj, index, id); // prot
  index_item(obj, index, id); // tag
  index_attributes(obj, index, id);
}

void index_weakref(unpack_data_t *obj, rds_index_mutable_t *index, size_t id) {
  index->objects[id].length = 1;
  add_read_index_ref(obj, index, id);
  index_attributes(obj, index, id);
}

// almost the same as the index_persistent_character, but with one
// extra integer read and no attribute check
void index_persistent_string(unpack_data_t *obj, rds_index_mutable_t *index, size_t id) {
  if (buffer_read_integer(obj->buffer) != 0) {
    // This is an R limitation
    Rf_error("names in persistent strings are not supported yet");
  }
  R_xlen_t len = buffer_read_integer(obj->buffer); // or store in obj?
  index->objects[id].length = len;
  index->objects[id].start_data = obj->buffer->pos;
  for (R_xlen_t i = 0; i < len; ++i) {
    index_item(obj, index, id);
  }
  index->objects[id].start_attr = obj->buffer->pos;
  index->objects[id].end = obj->buffer->pos;
}

void index_attributes(unpack_data_t *obj, rds_index_mutable_t *index, size_t id) {
  sexp_info_t *info = index->objects + id;
  info->start_attr = obj->buffer->pos;
  if (info->type == CHARSXP) {
    if (info->has_attr) {
      Rf_error("WRITEME (attributes on charsxp)");
      // unpack_read_item(obj);
    }
  } else if (info->has_attr) {
    index_item(obj, index, id);
  }
  index->objects[id].end = obj->buffer->pos;
}

// The process for inserting a ref during indexing is a little more
// complicated.
//
// id = 20, count = 2
//
// set ref_table[2] = 20
// set index[20].refid = 2
//
// resolve
//
// read 2
// ref_table[2] ==>20
// index.refid = 20
//
// Lookup
//
// .refid ==> 20
// index[20].refid ==> 2
// references[2] ==> SEXP

// MakeReadRefTable
SEXP init_read_index_ref() {
  SEXP ret;
  size_t initial_size = INITIAL_REFREAD_TABLE_SIZE;
  SEXP data = PROTECT(allocVector(INTSXP, initial_size));
  ret = CONS(data, R_NilValue);
  UNPROTECT(1);
  return ret;
}

// GetReadRef
int get_read_index_ref(unpack_data_t *obj, rds_index_mutable_t *index, int idx) {
  SEXP data = CAR(obj->ref_objects);
  int i = idx - 1;
  if (i < 0 || (size_t)i >= index->n_refs) {
    Rf_error("reference index out of range");
  }
  size_t refid = INTEGER(data)[i];
  Rprintf("Retrieving reference %d (refid %d)\n", i, refid);
  return refid;
}

// AddReadRef
void add_read_index_ref(unpack_data_t *obj, rds_index_mutable_t *index, size_t id) {
  // used by:
  // - [x] SYMSXP
  // - [ ] PERSISTSXP
  // - [ ] PACKAGESXP
  // - [ ] NAMESPACESXP
  // - [ ] ENVSXP
  // - [ ] EXTPTRSXP
  // - [ ] WEAKREFSXP
  int count = index->n_refs++;
  SEXP data = CAR(obj->ref_objects);
  if (count >= LENGTH(data)) {
    Rprintf("Growing reference index\n");
    int len = 2 * count;
    SEXP newdata = allocVector(INTSXP, len);
    memcpy(INTEGER(newdata), INTEGER(data), LENGTH(data) * sizeof(int));
    data = newdata;
  }
  index->objects[id].refid = count + 1;
  INTEGER(data)[count] = id;
  Rprintf("Inserting reference %d (refid %d)\n", count, id);
}

// Used by rdsi:
SEXP index_as_matrix(const rds_index_t *index) {
  const size_t n = index->len, nc = 9;

  SEXP ret = PROTECT(allocMatrix(INTSXP, n, nc));
  int
    *id           = INTEGER(ret),
    *parent       = INTEGER(ret) + n,
    *type         = INTEGER(ret) + n * 2,
    *length       = INTEGER(ret) + n * 3,
    *start_object = INTEGER(ret) + n * 4,
    *start_data   = INTEGER(ret) + n * 5,
    *start_attr   = INTEGER(ret) + n * 6,
    *end          = INTEGER(ret) + n * 7,
    *refid        = INTEGER(ret) + n * 8;
  SEXP nms = PROTECT(allocVector(STRSXP, nc));
  SET_STRING_ELT(nms, 0, mkChar("id"));
  SET_STRING_ELT(nms, 1, mkChar("parent"));
  SET_STRING_ELT(nms, 2, mkChar("type"));
  SET_STRING_ELT(nms, 3, mkChar("length"));
  SET_STRING_ELT(nms, 4, mkChar("start_object"));
  SET_STRING_ELT(nms, 5, mkChar("start_data"));
  SET_STRING_ELT(nms, 6, mkChar("start_attr"));
  SET_STRING_ELT(nms, 7, mkChar("end"));
  SET_STRING_ELT(nms, 8, mkChar("refid"));
  SEXP dns = PROTECT(allocVector(VECSXP, 2));
  SET_VECTOR_ELT(dns, 1, nms);
  setAttrib(ret, R_DimNamesSymbol, dns);

  for (size_t i = 0; i < n; ++i) {
    const sexp_info_t *info = index->objects + i;
    id[i]           = i;
    parent[i]       = info->parent;
    type[i]         = info->type;
    length[i]       = info->length;
    start_object[i] = info->start_object;
    start_data[i]   = info->start_data;
    start_attr[i]   = info->start_attr;
    end[i]          = info->end;
    refid[i]        = info->refid;
  }

  setAttrib(ret, install("n_refs"), ScalarInteger(index->n_refs));
  setAttrib(ret, install("len_data"), ScalarInteger(index->len_data));

  UNPROTECT(3);
  return ret;
}
