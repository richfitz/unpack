#include "index.h"
#include "util.h"

static void r_index_finalize(SEXP r_ptr);

SEXP r_unpack_index(SEXP x, SEXP r_as_ptr) {
  bool as_ptr = scalar_logical(r_as_ptr, "as_ptr");
  struct stream_st stream;
  unpack_prepare(x, &stream);

  rds_index *index = Calloc(1, rds_index);
  SEXP ret = PROTECT(R_MakeExternalPtr(index, R_NilValue, R_NilValue));
  R_RegisterCFinalizer(ret, r_index_finalize);

  index_init(index, 100);
  index_build(&stream, index, 0);
  if (stream.count != stream.size) {
    Rf_error("Did not consume all of raw vector: %d bytes left",
             stream.size - stream.count);
  }

  if (!as_ptr) {
    ret = r_unpack_index_as_matrix(ret);
  }

  UNPROTECT(1);
  return ret;
}

SEXP r_unpack_index_as_matrix(SEXP r_ptr) {
  rds_index * index = (rds_index*)R_ExternalPtrAddr(r_ptr);
  if (index == NULL) {
    Rf_error("index has been freed; can't use!");
  }
  return index_return(index);
}

void r_index_finalize(SEXP r_ptr) {
  rds_index * index = (rds_index*)R_ExternalPtrAddr(r_ptr);
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
  }

  index->index = (sexp_info*) Realloc(index->index, index->len, sexp_info);

  /*
  const size_t m = (IDX_END + 1) * index->len;
  const size_t len = m * index->len;
  if (index->index == NULL) {
    index->index = (int*) R_alloc(len, sizeof(int));
  } else {
    index->len *= 2;
    int * prev = index->index;
    index->index = (int*) R_alloc(m * index->len, sizeof(int));
    memcpy(index->index, prev, len * sizeof(int));
  }
  */
}

SEXP index_return(rds_index *index) {
  const size_t n = index->id, nc = 8;

  // It's not clear if this is better done with a matrix or with a
  // list that will be workable as a data.frame.  The other thing that
  // needs toing is swapping out the int here for double when the
  // length is long.
  SEXP ret = PROTECT(allocMatrix(INTSXP, n, nc));
  int
    *id           = INTEGER(ret),
    *parent       = INTEGER(ret) + n,
    *type         = INTEGER(ret) + n * 2,
    *length       = INTEGER(ret) + n * 3,
    *start_object = INTEGER(ret) + n * 4,
    *start_data   = INTEGER(ret) + n * 5,
    *start_attr   = INTEGER(ret) + n * 6,
    *end          = INTEGER(ret) + n * 7;
  SEXP nms = PROTECT(allocVector(STRSXP, nc));
  SET_STRING_ELT(nms, 0, mkChar("id"));
  SET_STRING_ELT(nms, 1, mkChar("parent"));
  SET_STRING_ELT(nms, 2, mkChar("type"));
  SET_STRING_ELT(nms, 3, mkChar("length"));
  SET_STRING_ELT(nms, 4, mkChar("start_object"));
  SET_STRING_ELT(nms, 5, mkChar("start_data"));
  SET_STRING_ELT(nms, 6, mkChar("start_attr"));
  SET_STRING_ELT(nms, 7, mkChar("end"));
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
  }

  UNPROTECT(3);
  return ret;
}

// TODO: index_build -> index_item?
void index_build(stream_t stream, rds_index *index, size_t parent) {
  if (index->id == index->len) {
    index_grow(index);
  }
  size_t id = index->id;
  sexp_info *info = index->index + index->id;
  info->start_object = stream->count;
  unpack_flags(stream_read_integer(stream), info);
  info->parent = parent;
  info->start_data = stream->count;
  index->id++;

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
    info->start_data = stream->count;
    info->start_attr = stream->count;
    info->end = stream->count;
    return;
    // 2. Reference objects; none of these are handled (yet? ever?)
  case REFSXP:
  case PERSISTSXP:
  case PACKAGESXP:
  case NAMESPACESXP:
  case ENVSXP:
    Rf_error("Can't unpack objects of type %s",
             CHAR(type2str(info->type)));
    // 3. Symbols
  case SYMSXP:
    index_symbol(stream, index, id);
    return;
    // 4. Dotted pair lists
  case LISTSXP:
  case LANGSXP:
  case CLOSXP:
  case PROMSXP:
  case DOTSXP:
    index_pairlist(stream, index, id);
    return;
    // 5. References
  case EXTPTRSXP:  // +attr
  case WEAKREFSXP: // +attr
    // 6. Special functions
  case SPECIALSXP: // +attr
  case BUILTINSXP: // +attr
    Rf_error("Can't unpack objects of type %s",
             CHAR(type2str(info->type)));
    // 7. Single string
  case CHARSXP: // +attr
    index_charsxp(stream, index, id);
    return;
    // 8. Vectors!
  case LGLSXP: // +attr
  case INTSXP: // +attr
    index_vector(stream, index, sizeof(int), id);
    return;
  case REALSXP: // +attr
    index_vector(stream, index, sizeof(double), id);
    return;
  case CPLXSXP: // +attr
    index_vector(stream, index, sizeof(Rcomplex), id);
    return;
  case STRSXP: // +attr
    index_vector_character(stream, index, id);
    return;
  case VECSXP: // +attr
  case EXPRSXP: // +attr
    index_vector_generic(stream, index, id);
    return;
    // 9. Weird shit
  case BCODESXP: // +attr
  case CLASSREFSXP: // +attr
  case GENERICREFSXP: // +attr
    Rf_error("Can't unpack objects of type %s",
             CHAR(type2str(info->type)));
    // 10. More vectors
  case RAWSXP: // +attr
    index_vector(stream, index, sizeof(char), id);
    return;
    // return stream_read_vector_raw(stream, &info);
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

void index_vector(stream_t stream, rds_index *index,
                  size_t element_size, size_t id) {
  sexp_info *info = index->index + id;
  info->length = stream_read_length(stream);
  info->start_data = stream->count;
  switch (stream->format) {
  case BINARY:
    stream_advance(stream, element_size * info->length);
    break;
  case ASCII:
  case XDR:
  default:
    Rf_error("not implemented (index_vector_real)");
  }
  index_attributes(stream, index, id);
}

void index_charsxp(stream_t stream, rds_index *index, size_t id) {
  sexp_info *info = index->index + id;
  // NOTE: *not* read_length() because limited to 2^32 - 1
  info->length = stream_read_integer(stream);
  info->start_data = stream->count;
  if (info->length > 0) {
    stream_advance(stream, info->length);
  }
  index_attributes(stream, index, id);
}

void index_symbol(stream_t stream, rds_index *index, size_t id) {
  sexp_info *info = index->index + id;
  info->length = 1;
  info->start_data = stream->count;
  stream->depth++;
  index_build(stream, index, id);
  stream->depth--;
  info->start_attr = stream->count;
  info->end = stream->count;
}

void index_vector_character(stream_t stream, rds_index *index, size_t id) {
  index_vector_generic(stream, index, id);
}

void index_vector_generic(stream_t stream, rds_index *index, size_t id) {
  sexp_info *info = index->index + id;
  info->length = stream_read_length(stream);
  info->start_data = stream->count;
  stream->depth++;
  for (R_xlen_t i = 0; i < info->length; ++i) {
    index_build(stream, index, id);
  }
  stream->depth--;
  index_attributes(stream, index, id);
}

void index_attributes(stream_t stream, rds_index *index, size_t id) {
  sexp_info *info = index->index + id;
  info->start_attr = stream->count;
  if (info->type == CHARSXP) {
    if (info->has_attr) {
      stream->depth++;
      Rf_error("WRITEME (attributes on charsxp)");
      // unpack_read_item(stream);
      stream->depth--;
    }
  } else if (info->has_attr) {
    stream->depth++;
    index_build(stream, index, id);
    stream->depth--;
  }
  info->end = stream->count;
}

void index_pairlist(stream_t stream, rds_index *index, size_t id) {
  sexp_info *info = index->index + id;
  info->length = 1; // pairlist always have one element
  info->start_attr = stream->count;
  stream->depth++;
  if (info->has_attr) {
    index_build(stream, index, id);
  }
  // We index this but it will not sensibly appear in some ways.  We
  // could infer it from the combination of has_tag and has_attr and
  // the locations of the pointers but it will be quite hard.  So
  // getting the attribute out will be easy but not the cdr.  I'm not
  // really sure that this is rich enough to help us index into
  // pairlists actually.
  if (info->has_tag) {
    index_build(stream, index, id);
  }
  index_build(stream, index, id); // car
  stream->depth--;
  info->start_data = stream->count;
  index_build(stream, index, id); // cdr
  info->end = stream->count;
}
