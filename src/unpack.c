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
void stream_read_vector_integer(stream_t stream, SEXP dest, R_xlen_t len) {
  switch (stream->format) {
  case BINARY:
    stream_read_bytes(stream, INTEGER(dest), (size_t)(sizeof(int) * len));
    break;
  case ASCII:
  case XDR:
  default:
    Rf_error("not implemented (read_vector_integer)");
  }
}

// InRealVec
void stream_read_vector_real(stream_t stream, SEXP dest, R_xlen_t len) {
  switch (stream->format) {
  case BINARY:
    stream_read_bytes(stream, REAL(dest), (size_t)(sizeof(double) * len));
    break;
  case ASCII:
  case XDR:
  default:
    Rf_error("not implemented (read_vector_real)");
  }
}

// InComplexVec
void stream_read_vector_complex(stream_t stream, SEXP dest, R_xlen_t len) {
  switch (stream->format) {
  case BINARY:
    stream_read_bytes(stream, COMPLEX(dest), (size_t)(sizeof(Rcomplex) * len));
    break;
  case ASCII:
  case XDR:
  default:
    Rf_error("not impemented (read_vector_complex)");
  }
}

// ...no analog
void stream_read_vector_raw(stream_t stream, SEXP dest, R_xlen_t len) {
  stream_read_bytes(stream, RAW(dest), (size_t)len);
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
  SEXP s;
  sexp_info info;
  unpack_flags(stream_read_integer(stream), &info);

  switch (info.type) {
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
  case REFSXP:
  case PERSISTSXP:
  case SYMSXP:
  case PACKAGESXP:
  case NAMESPACESXP:
  case ENVSXP:
  case LISTSXP:
  case LANGSXP:
  case CLOSXP:
  case PROMSXP:
  case DOTSXP:
    Rf_error("Can't unpack objects of type %s",
             CHAR(type2str(info.type)));
  default:
    /* These break out of the switch to have their ATTR,
       LEVELS, and OBJECT fields filled in.  Each leaves the
       newly allocated value PROTECTed */
    switch (info.type) {
    case EXTPTRSXP:
    case WEAKREFSXP:
    case SPECIALSXP:
    case BUILTINSXP:
    case CHARSXP:
      s = stream_read_charsxp(stream, &info);
      break;
    case LGLSXP:
    case INTSXP:
      info.length = stream_read_length(stream);
      PROTECT(s = allocVector(info.type, info.length));
      stream_read_vector_integer(stream, s, info.length);
      break;
    case REALSXP:
      info.length = stream_read_length(stream);
      PROTECT(s = allocVector(info.type, info.length));
      stream_read_vector_real(stream, s, info.length);
      break;
    case CPLXSXP:
      info.length = stream_read_length(stream);
      PROTECT(s = allocVector(info.type, info.length));
      stream_read_vector_complex(stream, s, info.length);
      break;
    case STRSXP:
      s = stream_read_vector_character(stream, &info);
      break;
    case VECSXP:
    case EXPRSXP:
      s = stream_read_vector_generic(stream, &info);
      break;
    case BCODESXP:
    case CLASSREFSXP:
    case GENERICREFSXP:
      Rf_error("Can't unpack objects of type %s",
               CHAR(type2str(info.type)));
    case RAWSXP:
      info.length = stream_read_length(stream);
      PROTECT(s = allocVector(info.type, info.length));
      stream_read_vector_raw(stream, s, info.length);
      break;
    case S4SXP:
      Rf_error("implement this");
      break;
    default:
      Rf_error("Can't unpack objects of type %s",
               CHAR(type2str(info.type)));
    }
    if (info.type != CHARSXP) {
      SETLEVELS(s, info.levels);
    }
    SET_OBJECT(s, info.is_object);
    if (TYPEOF(s) == CHARSXP) {
      if (info.has_attr) {
        stream->depth++;
        unpack_read_item(stream);
        stream->depth--;
      }
    } else {
      stream->depth++;
      SET_ATTRIB(s, info.has_attr ? unpack_read_item(stream) : R_NilValue);
      stream->depth--;
    }
  }
  UNPROTECT(1);
  return s;
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
