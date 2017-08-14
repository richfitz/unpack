#include "index.h"
#include "util.h"

// TODO: start_data is set in two different places here - for
// everything in index_build() just after setting refid, but also
// within some of the functions.  It's not clear that this is being
// done that badly, but it feels like there's some repetition here.

static void r_index_finalize(SEXP r_ptr);

SEXP r_unpack_index(SEXP x, SEXP r_as_ptr) {
  bool as_ptr = scalar_logical(r_as_ptr, "as_ptr");
  unpack_data *obj = unpack_data_prepare(x);

  rds_index *index = Calloc(1, rds_index);
  SEXP ret = PROTECT(R_MakeExternalPtr(index, R_NilValue, R_NilValue));
  R_RegisterCFinalizer(ret, r_index_finalize);
  index_init(index, 100);
  obj->index = index;
  init_read_index_ref(obj);

  index_build(obj, 0);
  stream_check_empty(obj->stream);
  index->ref_table = NULL;

  if (!as_ptr) {
    ret = r_unpack_index_as_matrix(ret);
  }

  UNPROTECT(1);
  return ret;
}

SEXP r_unpack_index_as_matrix(SEXP r_ptr) {
  return index_return(get_index(r_ptr, true));
}

void r_index_finalize(SEXP r_ptr) {
  rds_index * index = get_index(r_ptr, false);
  if (index == NULL) {
    Free(index->index);
    Free(index);
    R_ClearExternalPtr(r_ptr);
  }
}

void index_init(rds_index *index, size_t n) {
  index->id = 0;
  index->len = 100; // initial length
  index->index = NULL;
  index_grow(index);
}

// We'll put the memory here into something with a finaliser
void index_grow(rds_index *index) {
  if (index->index != NULL) {
    index->len *= 2;
    // NOTE: when doing grow, we need to re-lookup all indices that
    // are live.  So index_build could return true/false or we need to
    // be careful when storing pointers from index.
  }
  index->index = (sexp_info*) Realloc(index->index, index->len, sexp_info);
}

// This is primarily for inspection in R.  Some care will be needed
// where the sizes exceed 2^31 - 1; we can check that with looking at
// the end.
SEXP index_return(rds_index *index) {
  const size_t n = index->id, nc = 9;

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
    sexp_info *info = index->index + i;
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

  UNPROTECT(3);
  return ret;
}

// TODO: index_build -> index_item?
void index_build(unpack_data *obj, size_t parent) {
  if (obj->index->id == obj->index->len) {
    index_grow(obj->index);
  }
  size_t id = obj->index->id;
  sexp_info *info = obj->index->index + obj->index->id;
  info->start_object = obj->stream->count;
  unpack_flags(unpack_read_integer(obj), info);
  info->parent = parent;
  info->refid = 0; // this is a lie but it does not matter
  info->start_data = obj->stream->count;
  obj->index->id++;

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
    info->start_attr = obj->stream->count;
    info->end = obj->stream->count;
    return;
    // 2. Reference objects; none of these are handled (yet? ever?)
  case REFSXP:
    index_ref(obj, id);
    return;
  case PERSISTSXP:
    Rf_error("unimplemented: index_persistent");
  case PACKAGESXP:
    index_package(obj, id);
    return;
  case NAMESPACESXP:
    index_namespace(obj, id);
    return;
  case ENVSXP:
    index_environment(obj, id);
    return;
    // 3. Symbols
  case SYMSXP:
    index_symbol(obj, id);
    return;
    // 4. Dotted pair lists
  case LISTSXP:
  case LANGSXP:
  case CLOSXP:
  case PROMSXP:
  case DOTSXP:
    index_pairlist(obj, id);
    return;
    // 5. References
  case EXTPTRSXP:  // +attr
    index_extptr(obj, id);
    return;
  case WEAKREFSXP: // +attr
    index_weakref(obj, id);
    return;
    // 6. Special functions
  case SPECIALSXP: // +attr
  case BUILTINSXP: // +attr
    Rf_error("Can't unpack objects of type %s",
             CHAR(type2str(info->type)));
    // 7. Single string
  case CHARSXP: // +attr
    index_charsxp(obj, id);
    return;
    // 8. Vectors!
  case LGLSXP: // +attr
  case INTSXP: // +attr
    index_vector(obj, id, sizeof(int));
    return;
  case REALSXP: // +attr
    index_vector(obj, id, sizeof(double));
    return;
  case CPLXSXP: // +attr
    index_vector(obj, id, sizeof(Rcomplex));
    return;
  case STRSXP: // +attr
    index_vector_character(obj, id);
    return;
  case VECSXP: // +attr
  case EXPRSXP: // +attr
    index_vector_generic(obj, id);
    return;
    // 9. Weird shit
  case BCODESXP: // +attr
  case CLASSREFSXP: // +attr
  case GENERICREFSXP: // +attr
    Rf_error("Can't unpack objects of type %s",
             CHAR(type2str(info->type)));
    // 10. More vectors
  case RAWSXP: // +attr
    index_vector(obj, id, sizeof(char));
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

void index_vector(unpack_data *obj, size_t id, size_t element_size) {
  sexp_info *info = obj->index->index + id;
  info->length = unpack_read_length(obj);
  info->start_data = obj->stream->count;
  switch (obj->stream->format) {
  case BINARY:
    stream_advance(obj->stream, element_size * info->length);
    break;
  case ASCII:
  case XDR:
  default:
    Rf_error("not implemented (index_vector_real)");
  }
  index_attributes(obj, id);
}

void index_charsxp(unpack_data *obj, size_t id) {
  sexp_info *info = obj->index->index + id;
  // NOTE: *not* read_length() because limited to 2^32 - 1
  info->length = unpack_read_integer(obj);
  info->start_data = obj->stream->count;
  if (info->length > 0) {
    stream_advance(obj->stream, info->length);
  }
  index_attributes(obj, id);
}

void index_ref(unpack_data *obj, size_t id) {
  sexp_info *info = obj->index->index + id;
  int idx = unpack_read_ref_index(obj, info);
  info->length = 0;
  info->start_data = obj->stream->count;
  info->start_attr = obj->stream->count;
  info->end = obj->stream->count;
  info->refid = get_read_index_ref(obj, idx);
}

void index_package(unpack_data *obj, size_t id) {
  index_persistent_string(obj, id);
  add_read_index_ref(obj, id);
}

void index_namespace(unpack_data *obj, size_t id) {
  index_persistent_string(obj, id);
  add_read_index_ref(obj, id);
}

void index_environment(unpack_data *obj, size_t id) {
  obj->index->index[id].length = 1;
  unpack_read_integer(obj); // locked/unlocked (TODO: advance_integer instead?)
  add_read_index_ref(obj, id);
  obj->index->index[id].start_data = obj->stream->count;
  obj->stream->depth++;
  index_build(obj, id); // enclos
  index_build(obj, id); // frame
  index_build(obj, id); // hashtab
  obj->index->index[id].start_attr = obj->stream->count;
  index_build(obj, id); // attrib
  obj->stream->depth--;
  obj->index->index[id].end = obj->stream->count;
}

void index_extptr(unpack_data *obj, size_t id) {
  obj->index->index[id].length = 1;
  add_read_index_ref(obj, id);
  index_build(obj, id); // prot
  index_build(obj, id); // tag
  index_attributes(obj, id);
}

void index_weakref(unpack_data *obj, size_t id) {
  obj->index->index[id].length = 1;
  add_read_index_ref(obj, id);
  index_attributes(obj, id);
}

void index_symbol(unpack_data *obj, size_t id) {
  obj->index->index[id].length = 1;
  obj->stream->depth++;
  index_build(obj, id);
  obj->stream->depth--;
  obj->index->index[id].start_attr = obj->stream->count;
  obj->index->index[id].end = obj->stream->count;
  add_read_index_ref(obj, id);
}

void index_vector_character(unpack_data *obj, size_t id) {
  index_vector_generic(obj, id);
}

void index_vector_generic(unpack_data *obj, size_t id) {
  obj->index->index[id].length = unpack_read_length(obj);
  obj->index->index[id].start_data = obj->stream->count;
  obj->stream->depth++;
  for (R_xlen_t i = 0; i < obj->index->index[id].length; ++i) {
    index_build(obj, id);
  }
  obj->stream->depth--;
  index_attributes(obj, id);
}

// almost the same as the index_persistent_character, but with one
// extra integer read and no attribute check
void index_persistent_string(unpack_data *obj, size_t id) {
  if (unpack_read_integer(obj) != 0) {
    // This is an R limitation
    Rf_error("names in persistent strings are not supported yet");
  }
  R_xlen_t len = unpack_read_integer(obj); // or store in obj?
  obj->index->index[id].length = len;
  obj->index->index[id].start_data = obj->stream->count;
  obj->stream->depth++;
  for (R_xlen_t i = 0; i < len; ++i) {
    index_build(obj, id);
  }
  obj->stream->depth--;
  obj->index->index[id].start_attr = obj->stream->count;
  obj->index->index[id].end = obj->stream->count;
}

void index_attributes(unpack_data *obj, size_t id) {
  sexp_info *info = obj->index->index + id;
  info->start_attr = obj->stream->count;
  if (info->type == CHARSXP) {
    if (info->has_attr) {
      obj->stream->depth++;
      Rf_error("WRITEME (attributes on charsxp)");
      // unpack_read_item(obj);
      obj->stream->depth--;
    }
  } else if (info->has_attr) {
    obj->stream->depth++;
    index_build(obj, id);
    obj->stream->depth--;
  }
  obj->index->index[id].end = obj->stream->count;
}

void index_pairlist(unpack_data *obj, size_t id) {
  obj->index->index[id].length = 1; // pairlist always have one element
  obj->index->index[id].start_attr = obj->stream->count;
  obj->stream->depth++;
  if (obj->index->index[id].has_attr) {
    index_build(obj, id);
  }
  // We index this but it will not sensibly appear in some ways.  We
  // could infer it from the combination of has_tag and has_attr and
  // the locations of the pointers but it will be quite hard.  So
  // getting the attribute out will be easy but not the cdr.  I'm not
  // really sure that this is rich enough to help us index into
  // pairlists actually.
  if (obj->index->index[id].has_tag) {
    index_build(obj, id);
  }
  index_build(obj, id); // car
  obj->stream->depth--;
  obj->index->index[id].start_data = obj->stream->count;
  index_build(obj, id); // cdr
  obj->index->index[id].end = obj->stream->count;
}

rds_index * get_index(SEXP r_ptr, bool closed_error) {
  rds_index * index = (rds_index*)R_ExternalPtrAddr(r_ptr);
  if (closed_error && index == NULL) {
    Rf_error("index has been freed; can't use!");
  }
  return index;
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
// .sameas ==> 20
// index[20].refid ==> 2
// references[2] ==> SEXP

// MakeReadRefTable
void init_read_index_ref(unpack_data *obj) {
  obj->index->ref_table_len = INITIAL_REFREAD_TABLE_SIZE;
  obj->index->ref_table_count = 0;
  obj->index->ref_table =
    (size_t*)R_alloc(obj->index->ref_table_len, sizeof(size_t));
}

// GetReadRef
int get_read_index_ref(unpack_data *obj, int idx) {
  int i = idx - 1;
  if (i < 0 || (size_t)i >= obj->index->ref_table_count) {
    Rf_error("reference index out of range");
  }
  size_t refid = obj->index->ref_table[i];
  Rprintf("Retrieving reference %d (refid %d)\n", i, refid);
  return refid;
}

// AddReadRef
void add_read_index_ref(unpack_data *obj, size_t id) {
  // used by:
  // - [x] SYMSXP
  // - [ ] PERSISTSXP
  // - [ ] PACKAGESXP
  // - [ ] NAMESPACESXP
  // - [ ] ENVSXP
  // - [ ] EXTPTRSXP
  // - [ ] WEAKREFSXP
  size_t count = obj->index->ref_table_count++;
  if (count >= obj->index->ref_table_len) {
    Rprintf("Growing reference index\n");
    obj->index->ref_table_len *= 2;
    size_t *prev = obj->index->ref_table;
    obj->index->ref_table =
      (size_t*) R_alloc(obj->index->ref_table_len, sizeof(size_t));
    memcpy(obj->index->ref_table, prev,
           obj->index->ref_table_len * sizeof(size_t));
  }
  obj->index->index[id].refid = count + 1;
  obj->index->ref_table[count] = id;
  Rprintf("Inserting reference %d (refid %d)\n", count, id);
}
