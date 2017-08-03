#include "unpack.h"

// TODO: I want a version of this that just returns a pointer to the
// bytes too and avoids all the copying?
// InBytes
void stream_read_bytes(stream_t stream, void *buf, size_t len) {
  if (stream->count + len > stream->size) {
    Rf_error("stream overflow");
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
    Rf_error("not impemented");
  }
  return NA_INTEGER;
}


void stream_advance(stream_t stream, size_t len) {
  if (stream->count + len > stream->size) {
    Rf_error("stream overflow");
  }
  stream->count += len;
}

SEXP r_unpack_all(SEXP x) {
  if (TYPEOF(x) != RAWSXP) {
    Rf_error("Expected a raw string");
  }

  // TODO: long vectors are not supported - don't allow them through
  struct stream_st stream;
  void *data = RAW(x);
  size_t len = LENGTH(x);

  stream.count = 0;
  stream.size = len;
  stream.buf = data;

  return unpack_unserialise(&stream);
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
  // return R_NilValue;
  return unpack_inspect_item(stream);
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

  size_t n = 6;
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

  UNPROTECT(2);
  return ret;
}
