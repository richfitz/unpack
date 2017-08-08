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

// InRefIndex
int stream_read_ref_index(stream_t stream, sexp_info *info) {
  int i = UNPACK_REF_INDEX(info->flags);
  if (i == 0) {
    return stream_read_integer(stream);
  } else {
    return i;
  }
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
  add_read_ref(stream->ref_table, s);
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

// ...no analog
SEXP stream_read_ref(stream_t stream, sexp_info *info) {
  int index = stream_read_ref_index(stream, info);
  return get_read_ref(stream->ref_table, index);
}

void stream_advance(stream_t stream, R_xlen_t len) {
  if (stream->count + len > stream->size) {
    Rf_error("stream overflow");
  }
  stream->count += len;
}

void stream_move_to(stream_t stream, R_xlen_t pos) {
  if (pos > stream->size) {
    Rf_error("stream overflow");
  }
  stream->count = pos;
}

SEXP r_unpack_all(SEXP x) {
  struct stream_st stream;
  unpack_prepare(x, &stream);
  stream.ref_table = PROTECT(init_read_ref());
  SEXP obj = unpack_read_item(&stream);
  if (stream.count != stream.size) {
    Rf_error("Did not consume all of raw vector: %d bytes left",
             stream.size - stream.count);
  }
  UNPROTECT(1);
  return obj;
}

SEXP r_unpack_inspect(SEXP x) {
  struct stream_st stream;
  unpack_prepare(x, &stream);
  return unpack_inspect_item(&stream);
}

SEXP r_sexptypes() {
  size_t n = 35;
  SEXP nms = PROTECT(allocVector(STRSXP, n));
  SEXP ret = PROTECT(allocVector(INTSXP, n));
  setAttrib(ret, R_NamesSymbol, nms);
  int *v = INTEGER(ret);

  size_t i = 0;

  v[i] = SYMSXP; // 1
  SET_STRING_ELT(nms, i++, mkChar("SYMSXP"));
  v[i] = LISTSXP; // 2
  SET_STRING_ELT(nms, i++, mkChar("LISTSXP"));
  v[i] = CLOSXP; // 3
  SET_STRING_ELT(nms, i++, mkChar("CLOSXP"));
  v[i] = ENVSXP; // 4
  SET_STRING_ELT(nms, i++, mkChar("ENVSXP"));
  v[i] = PROMSXP; // 5
  SET_STRING_ELT(nms, i++, mkChar("PROMSXP"));
  v[i] = LANGSXP; // 6
  SET_STRING_ELT(nms, i++, mkChar("LANGSXP"));
  v[i] = SPECIALSXP; // 7
  SET_STRING_ELT(nms, i++, mkChar("SPECIALSXP"));
  v[i] = BUILTINSXP; // 8
  SET_STRING_ELT(nms, i++, mkChar("BUILTINSXP"));
  v[i] = CHARSXP; // 9
  SET_STRING_ELT(nms, i++, mkChar("CHARSXP"));
  v[i] = LGLSXP; // 10
  SET_STRING_ELT(nms, i++, mkChar("LGLSXP"));
  v[i] = INTSXP; // 13
  SET_STRING_ELT(nms, i++, mkChar("INTSXP"));
  v[i] = REALSXP; // 14
  SET_STRING_ELT(nms, i++, mkChar("REALSXP"));
  v[i] = CPLXSXP; // 15
  SET_STRING_ELT(nms, i++, mkChar("CPLXSXP"));
  v[i] = STRSXP; // 16
  SET_STRING_ELT(nms, i++, mkChar("STRSXP"));
  v[i] = DOTSXP; // 17
  SET_STRING_ELT(nms, i++, mkChar("DOTSXP"));
  v[i] = VECSXP; // 19
  SET_STRING_ELT(nms, i++, mkChar("VECSXP"));
  v[i] = EXPRSXP; // 20
  SET_STRING_ELT(nms, i++, mkChar("EXPRSXP"));
  v[i] = BCODESXP; // 21
  SET_STRING_ELT(nms, i++, mkChar("BCODESXP"));
  v[i] = EXTPTRSXP; // 22
  SET_STRING_ELT(nms, i++, mkChar("EXTPTRSXP"));
  v[i] = WEAKREFSXP; // 23
  SET_STRING_ELT(nms, i++, mkChar("WEAKREFSXP"));
  v[i] = RAWSXP; // 24
  SET_STRING_ELT(nms, i++, mkChar("RAWSXP"));
  v[i] = S4SXP; // 25
  SET_STRING_ELT(nms, i++, mkChar("S4SXP"));
  v[i] = BASEENV_SXP; // 241
  SET_STRING_ELT(nms, i++, mkChar("BASEENV"));
  v[i] = EMPTYENV_SXP; // 242
  SET_STRING_ELT(nms, i++, mkChar("EMPTYENV"));
  v[i] = GENERICREFSXP; // 245
  SET_STRING_ELT(nms, i++, mkChar("GENERICREFSXP"));
  v[i] = CLASSREFSXP; // 246
  SET_STRING_ELT(nms, i++, mkChar("CLASSREFSXP"));
  v[i] = PERSISTSXP; // 247
  SET_STRING_ELT(nms, i++, mkChar("PERSISTSXP"));
  v[i] = PACKAGESXP; // 248
  SET_STRING_ELT(nms, i++, mkChar("PACKAGESXP"));
  v[i] = NAMESPACESXP; // 249
  SET_STRING_ELT(nms, i++, mkChar("NAMESPACESXP"));
  v[i] = BASENAMESPACE_SXP; // 250
  SET_STRING_ELT(nms, i++, mkChar("BASENAMESPACE"));
  v[i] = MISSINGARG_SXP; // 251
  SET_STRING_ELT(nms, i++, mkChar("MISSINGARG"));
  v[i] = UNBOUNDVALUE_SXP; // 252
  SET_STRING_ELT(nms, i++, mkChar("UNBOUNDVALUE"));
  v[i] = GLOBALENV_SXP; // 253
  SET_STRING_ELT(nms, i++, mkChar("GLOBALENV"));
  v[i] = NILVALUE_SXP; // 254
  SET_STRING_ELT(nms, i++, mkChar("NILVALUE"));
  v[i] = REFSXP; // 255
  SET_STRING_ELT(nms, i++, mkChar("REFSXP"));

  UNPROTECT(2);
  return ret;
}

SEXP r_to_sexptype(SEXP x) {
  int * v = INTEGER(x);
  SEXP ret = PROTECT(allocVector(STRSXP, length(x)));
  for (R_xlen_t i = 0; i < length(x); ++i) {
    const char *name = NULL;
    switch(v[i]) {
    case SYMSXP:            name = "SYMSXP";        break;
    case LISTSXP:           name = "LISTSXP";       break;
    case CLOSXP:            name = "CLOSXP";        break;
    case ENVSXP:            name = "ENVSXP";        break;
    case PROMSXP:           name = "PROMSXP";       break;
    case LANGSXP:           name = "LANGSXP";       break;
    case SPECIALSXP:        name = "SPECIALSXP";    break;
    case BUILTINSXP:        name = "BUILTINSXP";    break;
    case CHARSXP:           name = "CHARSXP";       break;
    case LGLSXP:            name = "LGLSXP";        break;
    case INTSXP:            name = "INTSXP";        break;
    case REALSXP:           name = "REALSXP";       break;
    case CPLXSXP:           name = "CPLXSXP";       break;
    case STRSXP:            name = "STRSXP";        break;
    case DOTSXP:            name = "DOTSXP";        break;
    case VECSXP:            name = "VECSXP";        break;
    case EXPRSXP:           name = "EXPRSXP";       break;
    case BCODESXP:          name = "BCODESXP";      break;
    case EXTPTRSXP:         name = "EXTPTRSXP";     break;
    case WEAKREFSXP:        name = "WEAKREFSXP";    break;
    case RAWSXP:            name = "RAWSXP";        break;
    case S4SXP:             name = "S4SXP";         break;
    case BASEENV_SXP:       name = "BASEENV";       break;
    case EMPTYENV_SXP:      name = "EMPTYENV";      break;
    case GENERICREFSXP:     name = "GENERICREFSXP"; break;
    case CLASSREFSXP:       name = "CLASSREFSXP";   break;
    case PERSISTSXP:        name = "PERSISTSXP";    break;
    case PACKAGESXP:        name = "PACKAGESXP";    break;
    case NAMESPACESXP:      name = "NAMESPACESXP";  break;
    case BASENAMESPACE_SXP: name = "BASENAMESPACE"; break;
    case MISSINGARG_SXP:    name = "MISSINGARG";    break;
    case UNBOUNDVALUE_SXP:  name = "UNBOUNDVALUE";  break;
    case GLOBALENV_SXP:     name = "GLOBALENV";     break;
    case NILVALUE_SXP:      name = "NILVALUE";      break;
    case REFSXP:            name = "REFSXP";        break;
    }
    SET_STRING_ELT(ret, i, name == NULL ? NA_STRING : mkChar(name));
  }
  UNPROTECT(1);
  return ret;
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
    // something like this:
    return stream_read_ref(stream, &info);
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
  unpack_sexp_info(stream, &info);
  if (info.type > 240) {
    switch (info.type) {
    case NILVALUE_SXP:
      info.type = NILSXP; break;
    case GLOBALENV_SXP:
      info.type = ENVSXP; break;
    }
  }

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

void unpack_sexp_info(stream_t stream, sexp_info *info) {
  unpack_flags(stream_read_integer(stream), info);

  // These are all the types with a concept of length.  Reading the
  // length does move the stream along to the beginning of the data
  // element of the sexp.
  switch (info->type) {
  case CHARSXP:
  case LGLSXP:
  case INTSXP:
  case REALSXP:
  case CPLXSXP:
  case STRSXP:
  case EXPRSXP:
  case VECSXP:
  case RAWSXP:
    info->length = stream_read_length(stream);
    break;
  case NILVALUE_SXP:
    info->length = 0;
    break;
  default:
    // This is of course incorrect for dotted lists but we can't at
    // this point compute the length.
    info->length = 1;
    break;
  };
}

// MakeReadRefTable
#define INITIAL_REFREAD_TABLE_SIZE 182
SEXP init_read_ref() {
  SEXP data = allocVector(VECSXP, INITIAL_REFREAD_TABLE_SIZE);
  SET_TRUELENGTH(data, 0);
  return CONS(data, R_NilValue);
}

// GetReadRef
SEXP get_read_ref(SEXP table, int index) {
  int i = index - 1;
  SEXP data = CAR(table);
  if (i < 0 || i >= LENGTH(data)) {
    Rf_error("reference index out of range");
  }
  return VECTOR_ELT(data, i);
}

// AddReadRef
void add_read_ref(SEXP table, SEXP value) {
  // used by:
  // - [x] SYMSXP
  // - [ ] PERSISTSXP
  // - [ ] PACKAGESXP
  // - [ ] NAMESPACESXP
  // - [ ] ENVSXP
  // - [ ] EXTPTRSXP
  // - [ ] WEAKREFSXP
  //
  // TODO: I do not think that the growth bits that I have here are R
  // API; TRUELENGTH and SET_TRUELENGTH - it might be instructive to
  // search github.com/cran for usage.
  SEXP data = CAR(table);
  int count = TRUELENGTH(data) + 1;
  if (count >= LENGTH(data)) {
    int len;
    SEXP newdata;

    PROTECT(value);
    len = 2 * count;
    newdata = allocVector(VECSXP, len);
    for (int i = 0; i < LENGTH(data); i++) {
      SET_VECTOR_ELT(newdata, i, VECTOR_ELT(data, i));
    }
    SETCAR(table, newdata);
    data = newdata;
    UNPROTECT(1);
  }
  SET_TRUELENGTH(data, count);
  SET_VECTOR_ELT(data, count - 1, value);
}
