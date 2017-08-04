#include "unpack.h"
#include "upstream.h"

// TODO: For now, assume the native byte order version and come back
// for the xdr version later (and ascii even later than that).

// InBytes
void stream_read_bytes(stream_t stream, void *buf, R_xlen_t len) {
  if (stream->count + len > stream->size) {
    Rf_error("stream overflow (trying to set %d of %d)",
             stream->count + len, stream->size);
  }
  memcpy(buf, stream->buf + stream->count, len);
  stream->count += len;
}

// InChar
int stream_read_char(stream_t stream) {
  if (stream->count >= stream->size) {
    Rf_error("stream overflow");
  }
  return stream->buf[stream->count++];
}

// InInteger
int stream_read_integer(stream_t stream) {
  // char word[128];
  // char buf[128];
  int i;
  switch (stream->format) {
  case BINARY:
    stream_read_bytes(stream, &i, sizeof(int));
    return i;
  case ASCII:
  case XDR:
  default:
    Rf_error("not implemented (read_integer)");
  }
  return NA_INTEGER;
}

// InString
void stream_read_string(stream_t stream, char *buf, int length) {
  switch (stream->format) {
  case BINARY:
  case XDR:
    stream_read_bytes(stream, buf, length);
    break;
  case ASCII:
  default:
    Rf_error("not implemented (read_string)");
  }
}

// ReadLENGTH
R_xlen_t stream_read_length(stream_t stream) {
  int len = stream_read_integer(stream);
#ifdef LONG_VECTOR_SUPPORT
  if (len < -1)
    Rf_error("negative serialized length for vector");
  if (len == -1) {
    unsigned int len1, len2;
    len1 = stream_read_integer(stream); /* upper part */
    len2 = stream_read_integer(stream); /* lower part */
    R_xlen_t xlen = len1;
    /* sanity check for now */
    if (len1 > 65536)
      Rf_error("invalid upper part of serialized vector length");
    return (xlen << 32) + len2;
  } else return len;
#else
  if (len < 0)
    Rf_error("negative serialized vector length:\nperhaps long vector from 64-bit version of R?");
  return len;
#endif
}

// InIntegerVec
SEXP stream_read_vector_integer(stream_t stream, sexp_info *info) {
  info->length = stream_read_length(stream);
  SEXP s = PROTECT(allocVector(info->type, info->length));
  switch (stream->format) {
  case BINARY:
    stream_read_bytes(stream, INTEGER(s), (size_t)(sizeof(int) * info->length));
    break;
  case ASCII:
  case XDR:
  default:
    Rf_error("not implemented (read_vector_integer)");
  }
  unpack_add_attributes(s, info, stream);
  UNPROTECT(1);
  return s;
}

// InRealVec
SEXP stream_read_vector_real(stream_t stream, sexp_info *info) {
  info->length = stream_read_length(stream);
  SEXP s = PROTECT(allocVector(info->type, info->length));
  switch (stream->format) {
  case BINARY:
    stream_read_bytes(stream, REAL(s),
                      (size_t)(sizeof(double) * info->length));
    break;
  case ASCII:
  case XDR:
  default:
    Rf_error("not implemented (read_vector_real)");
  }
  unpack_add_attributes(s, info, stream);
  UNPROTECT(1);
  return s;
}

// InComplexVec
SEXP stream_read_vector_complex(stream_t stream, sexp_info *info) {
  info->length = stream_read_length(stream);
  SEXP s = PROTECT(allocVector(info->type, info->length));
  switch (stream->format) {
  case BINARY:
    stream_read_bytes(stream, COMPLEX(s),
                      (size_t)(sizeof(Rcomplex) * info->length));
    break;
  case ASCII:
  case XDR:
  default:
    Rf_error("not impemented (read_vector_complex)");
  }
  unpack_add_attributes(s, info, stream);
  UNPROTECT(1);
  return s;
}

// ...no analog
SEXP stream_read_vector_raw(stream_t stream, sexp_info *info) {
  info->length = stream_read_length(stream);
  SEXP s = PROTECT(allocVector(info->type, info->length));
  stream_read_bytes(stream, RAW(s), (size_t)info->length);
  unpack_add_attributes(s, info, stream);
  UNPROTECT(1);
  return s;
}

// ...no analog
SEXP stream_read_vector_character(stream_t stream, sexp_info *info) {
  info->length = stream_read_length(stream);
  SEXP s = PROTECT(allocVector(info->type, info->length));
  stream->depth++;
  for (R_xlen_t i = 0; i < info->length; ++i) {
    SET_STRING_ELT(s, i, unpack_read_item(stream));
  }
  stream->depth--;
  unpack_add_attributes(s, info, stream);
  UNPROTECT(1);
  return s;
}

// ...no analog
SEXP stream_read_vector_generic(stream_t stream, sexp_info *info) {
  info->length = stream_read_length(stream);
  SEXP s = PROTECT(allocVector(info->type, info->length));
  stream->depth++;
  for (R_xlen_t i = 0; i < info->length; ++i) {
    SET_VECTOR_ELT(s, i, unpack_read_item(stream));
  }
  stream->depth--;
  unpack_add_attributes(s, info, stream);
  UNPROTECT(1);
  return s;
}

// ...no analog
SEXP stream_read_symbol(stream_t stream, sexp_info *info) {
  stream->depth++;
  SEXP s = PROTECT(unpack_read_item(stream));
  stream->depth--;
  // TODO: this _should_ be done with installTrChar which sets up the
  // translations.  However, we don't have access to that!  I don't
  // know if that will really hurt us if we don't have access to
  // reference objects - I presume that this is primarily for when
  // unserialising things that have come from packages with
  // translation support perhaps?  In any case we should be able to
  // re-implement
  //
  //   needsTranslation
  //
  // and warn if we're not able to do it.
  s = Rf_installChar(s);
  // In the R unserialise code we then add this symbol to the reference table.
  //
  // TODO: AddReadRef(ref_table, s);
  UNPROTECT(1);
  return s;
}

// ...no analog
SEXP stream_read_pairlist(stream_t stream, sexp_info *info) {
  // From serialize.c:
  /* This handling of dotted pair objects still uses recursion
     on the CDR and so will overflow the PROTECT stack for long
     lists.  The save format does permit using an iterative
     approach; it just has to pass around the place to write the
     CDR into when it is allocated.  It's more trouble than it
     is worth to write the code to handle this now, but if it
     becomes necessary we can do it without needing to change
     the save format. */
  // TODO: I do not know if allocSExp is API or not - if it's not the
  // we have to do some serious work...
  SEXP s = PROTECT(allocSExp(info->type));
  SETLEVELS(s, info->levels);
  SET_OBJECT(s, info->is_object);
  stream->depth++;
  SET_ATTRIB(s, info->has_attr ? unpack_read_item(stream) : R_NilValue);
  SET_TAG(s, info->has_tag ? unpack_read_item(stream) : R_NilValue);
  SETCAR(s, unpack_read_item(stream));
  stream->depth--;
  SETCDR(s, unpack_read_item(stream));
  UNPROTECT(1);
  return s;
}

// ...no analog
SEXP stream_read_charsxp(stream_t stream, sexp_info *info) {
  // NOTE: *not* read_length() because limited to 2^32 - 1
  info->length = stream_read_integer(stream);
  SEXP s;
  if (info->length == -1) {
    PROTECT(s = NA_STRING);
  } else if (info->length < 1000) {
    int enc = CE_NATIVE;
    char cbuf[info->length + 1];
    stream_read_string(stream, cbuf, info->length);
    cbuf[info->length] = '\0';
    // duplicated below
    if (info->levels & UTF8_MASK) {
      enc = CE_UTF8;
    } else if (info->levels & LATIN1_MASK) {
      enc = CE_LATIN1;
    } else if (info->levels & BYTES_MASK) {
      enc = CE_BYTES;
    }
    PROTECT(s = mkCharLenCE(cbuf, info->length, enc));
  } else {
    int enc = CE_NATIVE;
    char *cbuf = CallocCharBuf(info->length);
    // duplicated above
    stream_read_string(stream, cbuf, info->length);
    if (info->levels & UTF8_MASK) {
      enc = CE_UTF8;
    } else if (info->levels & LATIN1_MASK) {
      enc = CE_LATIN1;
    } else if (info->levels & BYTES_MASK) {
      enc = CE_BYTES;
    }
    PROTECT(s = mkCharLenCE(cbuf, info->length, enc));
    Free(cbuf);
  }
  unpack_add_attributes(s, info, stream);
  UNPROTECT(1);
  return s;
}

void stream_advance(stream_t stream, R_xlen_t len) {
  if (stream->count + len > stream->size) {
    Rf_error("stream overflow");
  }
  stream->count += len;
}

SEXP r_unpack_all(SEXP x) {
  struct stream_st stream;
  unpack_prepare(x, &stream);
  return unpack_read_item(&stream);
}

SEXP r_unpack_inspect(SEXP x) {
  struct stream_st stream;
  unpack_prepare(x, &stream);
  return unpack_inspect_item(&stream);
}

void unpack_prepare(SEXP x, stream_t stream) {
  if (TYPEOF(x) != RAWSXP) {
    Rf_error("Expected a raw string");
  }

  stream->count = 0;
  stream->size = LENGTH(x);
  stream->buf = RAW(x);
  stream->depth = 0;

  unpack_check_format(stream);
  unpack_check_version(stream);
}

SEXP unpack_unserialise(stream_t stream) {
  unpack_check_format(stream);
  unpack_check_version(stream);
  return unpack_read_item(stream);
}

void unpack_check_format(stream_t stream) {
  char buf[2];
  stream_read_bytes(stream, buf, 2);
  switch(buf[0]) {
  case 'A':
    stream->format = ASCII;
    break;
  case 'B':
    stream->format = BINARY;
    break;
  case 'X':
    stream->format = XDR;
    break;
  case '\n':
    stream->format = ASCII;
    stream_read_bytes(stream, buf, 1);
    break;
  default:
    Rf_error("Unknown input type");
  }
}

void unpack_check_version(stream_t stream) {
  int version, writer_version, release_version;
  version = stream_read_integer(stream);
  // Could just walk the reader along 2 * sizeof(int) bytes instead
  writer_version = stream_read_integer(stream);
  release_version = stream_read_integer(stream);
  if (version != 2) {
    Rf_error("Cannot read rds files in this format");
  }
}

// ReadItem
SEXP unpack_read_item(stream_t stream) {
  sexp_info info;
  unpack_flags(stream_read_integer(stream), &info);

  switch (info.type) {
    // 1. Some singletons:
  case NILVALUE_SXP:
    return R_NilValue;
  case EMPTYENV_SXP:
    return R_EmptyEnv;
  case BASEENV_SXP:
    return R_BaseEnv;
  case GLOBALENV_SXP:
    return R_GlobalEnv;
  case UNBOUNDVALUE_SXP:
    return R_UnboundValue;
  case MISSINGARG_SXP:
    return R_MissingArg;
  case BASENAMESPACE_SXP:
    return R_BaseNamespace;
    // 2. Reference objects; none of these are handled (yet? ever?)
  case REFSXP:
  case PERSISTSXP:
  case PACKAGESXP:
  case NAMESPACESXP:
  case ENVSXP:
    Rf_error("Can't unpack objects of type %s",
             CHAR(type2str(info.type)));
    // 3. Symbols
  case SYMSXP:
    return stream_read_symbol(stream, &info);
    // 4. Dotted pair lists
  case LISTSXP:
  case LANGSXP:
  case CLOSXP:
  case PROMSXP:
  case DOTSXP:
    return stream_read_pairlist(stream, &info);
    // 5. References
  case EXTPTRSXP:  // +attr
  case WEAKREFSXP: // +attr
    // 6. Special functions
  case SPECIALSXP: // +attr
  case BUILTINSXP: // +attr
    Rf_error("Can't unpack objects of type %s",
             CHAR(type2str(info.type)));
    // 7. Single string
  case CHARSXP: // +attr
    return stream_read_charsxp(stream, &info);
    // 8. Vectors!
  case LGLSXP: // +attr
  case INTSXP: // +attr
    return stream_read_vector_integer(stream, &info);
  case REALSXP: // +attr
    return stream_read_vector_real(stream, &info);
  case CPLXSXP: // +attr
    return stream_read_vector_complex(stream, &info);
  case STRSXP: // +attr
    return stream_read_vector_character(stream, &info);
  case VECSXP: // +attr
  case EXPRSXP: // +attr
    return stream_read_vector_generic(stream, &info);
    // 9. Weird shit
  case BCODESXP: // +attr
  case CLASSREFSXP: // +attr
  case GENERICREFSXP: // +attr
    Rf_error("Can't unpack objects of type %s",
             CHAR(type2str(info.type)));
    // 10. More vectors
  case RAWSXP: // +attr
    return stream_read_vector_raw(stream, &info);
    // 11. S4
  case S4SXP: // +attr
    Rf_error("implement this");
    break;
    // 12. Unknown types
  default:
      Rf_error("Can't unpack objects of type %s",
               CHAR(type2str(info.type)));
  }
  Rf_error("noreturn");
}

void unpack_add_attributes(SEXP s, sexp_info *info, stream_t stream) {
  if (info->type != CHARSXP) {
    SETLEVELS(s, info->levels);
  }
  SET_OBJECT(s, info->is_object);
  if (TYPEOF(s) == CHARSXP) {
    if (info->has_attr) {
      stream->depth++;
      unpack_read_item(stream);
      stream->depth--;
    }
  } else {
    stream->depth++;
    SET_ATTRIB(s, info->has_attr ? unpack_read_item(stream) : R_NilValue);
    stream->depth--;
  }
}

void unpack_flags(int flags, sexp_info * info) {
  info->flags = flags;
  info->type = DECODE_TYPE(flags);
  info->levels = DECODE_LEVELS(flags);
  info->is_object = flags & IS_OBJECT_BIT_MASK ? TRUE : FALSE;
  info->has_attr = flags & HAS_ATTR_BIT_MASK ? TRUE : FALSE;
  info->has_tag = flags & HAS_TAG_BIT_MASK ? TRUE : FALSE;
}

SEXP unpack_inspect_item(stream_t stream) {
  sexp_info info;
  unpack_flags(stream_read_integer(stream), &info);

  if (info.type > 240) {
    switch (info.type) {
    case NILVALUE_SXP:
      info.type = NILSXP; break;
    case GLOBALENV_SXP:
      info.type = ENVSXP; break;
    }
  }

  switch (info.type) {
  case NILSXP:
    info.length = 0;
    break;
  case CHARSXP:
  case LGLSXP:
  case INTSXP:
  case REALSXP:
  case CPLXSXP:
  case STRSXP:
  case EXPRSXP:
  case VECSXP:
  case RAWSXP:
    info.length = stream_read_length(stream);
    break;
  default:
    info.length = 1;
    break;
  };

  size_t n = 7;
  SEXP ret = PROTECT(allocVector(VECSXP, n));
  SEXP nms = PROTECT(allocVector(STRSXP, n));
  setAttrib(ret, R_NamesSymbol, nms);
  SET_VECTOR_ELT(ret, 0, ScalarInteger(info.flags));
  SET_STRING_ELT(nms, 0, mkChar("flags"));
  SET_VECTOR_ELT(ret, 1, ScalarString(Rf_type2str(info.type)));
  SET_STRING_ELT(nms, 1, mkChar("type"));
  SET_VECTOR_ELT(ret, 2, ScalarInteger(info.levels));
  SET_STRING_ELT(nms, 2, mkChar("levels"));
  SET_VECTOR_ELT(ret, 3, ScalarLogical(info.is_object));
  SET_STRING_ELT(nms, 3, mkChar("is_object"));
  SET_VECTOR_ELT(ret, 4, ScalarLogical(info.has_attr));
  SET_STRING_ELT(nms, 4, mkChar("has_attr"));
  SET_VECTOR_ELT(ret, 5, ScalarLogical(info.has_tag));
  SET_STRING_ELT(nms, 5, mkChar("has_tag"));
  // TODO: deal with long numbers here; might need to swich this out...
  SET_VECTOR_ELT(ret, 6, ScalarInteger(info.length));
  SET_STRING_ELT(nms, 6, mkChar("length"));

  UNPROTECT(2);
  return ret;
}
