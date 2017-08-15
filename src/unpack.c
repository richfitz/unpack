#include "unpack.h"
#include "upstream.h"

static SEXP R_FindNamespace1(SEXP info);

// TODO: For now, assume the native byte order version and come back
// for the xdr version later (and ascii even later than that).

// InBytes
void unpack_read_bytes(unpack_data *obj, void *buf, R_xlen_t len) {
  stream_t * stream = obj->stream;
  if (stream->count + len > stream->size) {
    Rf_error("stream overflow (trying to move to %d of %d)",
             stream->count + len, stream->size);
  }
  memcpy(buf, stream->buf + stream->count, len);
  stream->count += len;
}

// InChar
int unpack_read_char(unpack_data *obj) {
  stream_t * stream = obj->stream;
  if (stream->count >= stream->size) {
    Rf_error("stream overflow");
  }
  return stream->buf[stream->count++];
}

// InInteger
int unpack_read_integer(unpack_data *obj) {
  // char word[128];
  // char buf[128];
  int i;
  switch (obj->stream->format) {
  case BINARY:
    unpack_read_bytes(obj, &i, sizeof(int));
    return i;
  case ASCII:
  case XDR:
  default:
    Rf_error("not implemented (read_integer)");
  }
  return NA_INTEGER;
}

// InRefIndex
int unpack_read_ref_index(unpack_data *obj, sexp_info *info) {
  int i = UNPACK_REF_INDEX(info->flags);
  if (i == 0) {
    return unpack_read_integer(obj);
  } else {
    return i;
  }
}

SEXP unpack_read_package(unpack_data *obj, sexp_info *info) {
  SEXP s = PROTECT(unpack_read_persistent_string(obj, info));
  s = R_FindPackageEnv(s);
  UNPROTECT(1);
  add_read_ref(obj, s, info);
  return s;
}
SEXP unpack_read_namespace(unpack_data *obj, sexp_info *info) {
  SEXP s = PROTECT(unpack_read_persistent_string(obj, info));
  s = R_FindNamespace1(s);
  UNPROTECT(1);
  add_read_ref(obj, s, info);
  return s;
}

// This seems unlikely to be API, though it does all compile
SEXP unpack_read_environment(unpack_data *obj, sexp_info *info) {
  int locked = unpack_read_integer(obj);
  SEXP s = PROTECT(allocSExp(ENVSXP));
  add_read_ref(obj, s, info);

  obj->stream->depth++;
  SET_ENCLOS(s, unpack_read_item(obj));
  SET_FRAME(s, unpack_read_item(obj));
  SET_HASHTAB(s, unpack_read_item(obj));
  SET_ATTRIB(s, unpack_read_item(obj));
  obj->stream->depth--;

  if (ATTRIB(s) != R_NilValue &&
      getAttrib(s, R_ClassSymbol) != R_NilValue) {
    /* We don't write out the object bit for environments,
       so reconstruct it here if needed. */
    SET_OBJECT(s, 1);
  }
  R_RestoreHashCount(s);
  if (locked) {
    R_LockEnvironment(s, FALSE);
  }
  /* Convert a NULL enclosure to baseenv() */
  if (ENCLOS(s) == R_NilValue) {
    SET_ENCLOS(s, R_BaseEnv);
  }

  UNPROTECT(1);
  return s;
}

SEXP unpack_read_extptr(unpack_data *obj, sexp_info *info) {
  SEXP s = PROTECT(allocSExp(info->type));
  add_read_ref(obj, s, info);
  R_SetExternalPtrAddr(s, NULL);
  obj->stream->depth++;
  R_SetExternalPtrProtected(s, unpack_read_item(obj));
  R_SetExternalPtrTag(s, unpack_read_item(obj));
  obj->stream->depth--;
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
}

SEXP unpack_read_weakref(unpack_data *obj, sexp_info *info) {
  SEXP s = PROTECT(R_MakeWeakRef(R_NilValue, R_NilValue, R_NilValue,
                                 FALSE));
  add_read_ref(obj, s, info);
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
}

// There is close to zero chance that this is API
SEXP unpack_read_builtin(unpack_data *obj, sexp_info *info) {
  /*
  info->length = unpack_read_integer(obj);
  char cbuf[info->length + 1];
  unpack_read_string(obj, cbuf, info->length);
  cbuf[info->length] = '\0';
  int index = StrToInternal(cbuf);
  if (index == NA_INTEGER) {
    Rf_warning("unrecognized internal function name \"%s\"", cbuf);
    return R_NilValue;
  }
  PROTECT(s = mkPRIMSXP(index, type == BUILTINSXP));
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
  */
  Rf_error("unimplemented: unpack_read_builtin");
  return R_NilValue;
}

SEXP unpack_read_bcode(unpack_data *obj, sexp_info *info) {
  // This is another whole level of hell:
  /*
  info->length = unpack_read_integer(obj);
  SEXP reps = PROTECT(allocVector(VECSXP, info->length));
  SEXP s = ReadBC1(ref_table, reps, stream);
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
  */
  Rf_error("unimplemented: unpack_read_bcode");
  return R_NilValue;
}

// InString
void unpack_read_string(unpack_data *obj, char *buf, int length) {
  switch (obj->stream->format) {
  case BINARY:
  case XDR:
    unpack_read_bytes(obj, buf, length);
    break;
  case ASCII:
  default:
    Rf_error("not implemented (read_string)");
  }
}

// ReadLENGTH
R_xlen_t unpack_read_length(unpack_data *obj) {
  int len = unpack_read_integer(obj);
#ifdef LONG_VECTOR_SUPPORT
  if (len < -1)
    Rf_error("negative serialized length for vector");
  if (len == -1) {
    unsigned int len1, len2;
    len1 = unpack_read_integer(obj); /* upper part */
    len2 = unpack_read_integer(obj); /* lower part */
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
SEXP unpack_read_vector_integer(unpack_data *obj, sexp_info *info) {
  info->length = unpack_read_length(obj);
  SEXP s = PROTECT(allocVector(info->type, info->length));
  switch (obj->stream->format) {
  case BINARY:
    unpack_read_bytes(obj, INTEGER(s), (size_t)(sizeof(int) * info->length));
    break;
  case ASCII:
  case XDR:
  default:
    Rf_error("not implemented (read_vector_integer)");
  }
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
}

// InRealVec
SEXP unpack_read_vector_real(unpack_data *obj, sexp_info *info) {
  info->length = unpack_read_length(obj);
  SEXP s = PROTECT(allocVector(info->type, info->length));
  switch (obj->stream->format) {
  case BINARY:
    unpack_read_bytes(obj, REAL(s),
                      (size_t)(sizeof(double) * info->length));
    break;
  case ASCII:
  case XDR:
  default:
    Rf_error("not implemented (read_vector_real)");
  }
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
}

// InComplexVec
SEXP unpack_read_vector_complex(unpack_data *obj, sexp_info *info) {
  info->length = unpack_read_length(obj);
  SEXP s = PROTECT(allocVector(info->type, info->length));
  switch (obj->stream->format) {
  case BINARY:
    unpack_read_bytes(obj, COMPLEX(s),
                      (size_t)(sizeof(Rcomplex) * info->length));
    break;
  case ASCII:
  case XDR:
  default:
    Rf_error("not impemented (read_vector_complex)");
  }
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
}

// ...no analog
SEXP unpack_read_vector_raw(unpack_data *obj, sexp_info *info) {
  info->length = unpack_read_length(obj);
  SEXP s = PROTECT(allocVector(info->type, info->length));
  unpack_read_bytes(obj, RAW(s), (size_t)info->length);
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
}

// ...no analog
SEXP unpack_read_vector_character(unpack_data *obj, sexp_info *info) {
  info->length = unpack_read_length(obj);
  SEXP s = PROTECT(allocVector(info->type, info->length));
  obj->stream->depth++;
  for (R_xlen_t i = 0; i < info->length; ++i) {
    SET_STRING_ELT(s, i, unpack_read_item(obj));
  }
  obj->stream->depth--;
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
}

// InStringVec - almost the same as the above, but with one extra
// integer read and no attribute check, and type set explicitly
SEXP unpack_read_persistent_string(unpack_data *obj, sexp_info *info) {
  if (unpack_read_integer(obj) != 0) {
    // This is an R limitation
    Rf_error("names in persistent strings are not supported yet");
  }
  info->length = unpack_read_length(obj);
  SEXP s = PROTECT(allocVector(STRSXP, info->length));
  obj->stream->depth++;
  for (R_xlen_t i = 0; i < info->length; ++i) {
    SET_STRING_ELT(s, i, unpack_read_item(obj));
  }
  obj->stream->depth--;
  UNPROTECT(1);
  return s;
}

// ...no analog
SEXP unpack_read_vector_generic(unpack_data *obj, sexp_info *info) {
  info->length = unpack_read_length(obj);
  SEXP s = PROTECT(allocVector(info->type, info->length));
  obj->stream->depth++;
  for (R_xlen_t i = 0; i < info->length; ++i) {
    SET_VECTOR_ELT(s, i, unpack_read_item(obj));
  }
  obj->stream->depth--;
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
}

// ...no analog
SEXP unpack_read_symbol(unpack_data *obj, sexp_info *info) {
  obj->stream->depth++;
  SEXP s = PROTECT(unpack_read_item(obj));
  obj->stream->depth--;
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
  add_read_ref(obj, s, info);
  UNPROTECT(1);
  return s;
}

// ...no analog
SEXP unpack_read_pairlist(unpack_data *obj, sexp_info *info) {
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
  obj->stream->depth++;
  SET_ATTRIB(s, info->has_attr ? unpack_read_item(obj) : R_NilValue);
  SET_TAG(s, info->has_tag ? unpack_read_item(obj) : R_NilValue);
  SETCAR(s, unpack_read_item(obj));
  obj->stream->depth--;
  SETCDR(s, unpack_read_item(obj));
  UNPROTECT(1);
  return s;
}

// ...no analog
SEXP unpack_read_charsxp(unpack_data *obj, sexp_info *info) {
  // NOTE: *not* read_length() because limited to 2^32 - 1
  info->length = unpack_read_integer(obj);
  SEXP s;
  if (info->length == -1) {
    PROTECT(s = NA_STRING);
  } else if (info->length < 1000) {
    int enc = CE_NATIVE;
    char cbuf[info->length + 1];
    unpack_read_string(obj, cbuf, info->length);
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
    unpack_read_string(obj, cbuf, info->length);
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
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
}

// ...no analog
SEXP unpack_read_ref(unpack_data *obj, sexp_info *info) {
  // This one is much easier than insertion because we will just
  // arrange to be able to look the reference up.  Though if we do the
  // resolution recursively that won't happen (but I think that a plan
  // is a better bet).
  int index = unpack_read_ref_index(obj, info);
  return get_read_ref(obj, info, index);
}

SEXP unpack_read_persist(unpack_data *obj, sexp_info *info) {
  // PERSISTSXP
  SEXP s = PROTECT(unpack_read_persistent_string(obj, info));
  Rf_error("unimplemented: unpack_read_persistent");
  // s = PersistentRestore(stream, s);
  //  if (stream->InPersistHookFunc == NULL)
  //    error(_("no restore method available"));
  //  return stream->InPersistHookFunc(s, stream->InPersistHookData);
  UNPROTECT(1);
  add_read_ref(obj, s, info);
  return s;
}

void stream_advance(stream_t *stream, R_xlen_t len) {
  if (stream->count + len > stream->size) {
    Rf_error("stream overflow");
  }
  stream->count += len;
}

void stream_move_to(stream_t *stream, R_xlen_t pos) {
  if (pos > stream->size) {
    Rf_error("stream overflow");
  }
  stream->count = pos;
}

void stream_check_empty(stream_t *stream) {
  if (stream->count != stream->size) {
    Rf_error("Did not consume all of raw vector: %d bytes left",
             stream->size - stream->count);
  }
}

unpack_data * unpack_data_prepare(SEXP x) {
  unpack_data * obj = (unpack_data *)R_alloc(1, sizeof(unpack_data));
  unpack_prepare(x, obj);
  obj->ref_objects = R_NilValue;
  obj->index = NULL;
  obj->count = 0;
  return obj;
}

SEXP r_unpack_all(SEXP r_x) {
  unpack_data *obj = unpack_data_prepare(r_x);
  obj->ref_objects = PROTECT(init_read_ref(NULL));
  SEXP ret = PROTECT(unpack_read_item(obj));
  stream_check_empty(obj->stream);
  UNPROTECT(2);
  return ret;
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

void unpack_prepare(SEXP x, unpack_data *obj) {
  if (TYPEOF(x) != RAWSXP) {
    Rf_error("Expected a raw string");
  }

  obj->stream = (stream_t *)R_alloc(1, sizeof(stream_t));
  obj->stream->count = 0;
  obj->stream->size = LENGTH(x);
  obj->stream->buf = RAW(x);
  obj->stream->depth = 0;

  unpack_check_format(obj);
  unpack_check_version(obj);
}

void unpack_check_format(unpack_data *obj) {
  char buf[2];
  unpack_read_bytes(obj, buf, 2);
  switch(buf[0]) {
  case 'A':
    obj->stream->format = ASCII;
    break;
  case 'B':
    obj->stream->format = BINARY;
    break;
  case 'X':
    obj->stream->format = XDR;
    break;
  case '\n':
    obj->stream->format = ASCII;
    unpack_read_bytes(obj, buf, 1);
    break;
  default:
    Rf_error("Unknown input type");
  }
}

void unpack_check_version(unpack_data *obj) {
  int version, writer_version, release_version;
  version = unpack_read_integer(obj);
  // Could just walk the reader along 2 * sizeof(int) bytes instead
  writer_version = unpack_read_integer(obj);
  release_version = unpack_read_integer(obj);
  if (version != 2) {
    Rf_error("Cannot read rds files in this format");
  }
}

// ReadItem
SEXP unpack_read_item(unpack_data *obj) {
  size_t id = obj->count++;

  // First, if we have an index and if the object is a reference
  // object, have a go at pulling it from the set of resolved objects.
  if (obj->index != NULL) {
    // It would be more efficient to expose the bits that are needed
    // here in the index directly I think.  They're used in a couple
    // of places anyway.  But for now, this is OK and no worse than
    // what happens if we go all the way through the usual resolution.
    sexp_info *info_prev = obj->index->index + id;
    size_t refid = info_prev->refid;
    if (refid > 0 && info_prev->type != REFSXP) {
      if (INTEGER(CDR(obj->ref_objects))[refid - 1]) {
        Rprintf("(unpack_read_item) object %d (ref %d) already extracted\n",
                info_prev->id, refid);
        R_xlen_t end = info_prev->end;
        stream_move_to(obj->stream, end);
        // TODO: I don't *think* that a reference object can be the
        // last object in the rds, but this might be worth confirming
        // or checking here.
        do {
          info_prev++;
        } while (info_prev->end > end);
        obj->count = info_prev->id + 1;
        return VECTOR_ELT(CAR(obj->ref_objects), refid - 1);
      }
    }
  }

  sexp_info info;
  unpack_flags(unpack_read_integer(obj), &info);
  info.id = id;

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
    // 2. Reference objects
  case REFSXP:
    return unpack_read_ref(obj, &info);
  case PERSISTSXP: // this is done with the hook function
    return unpack_read_persist(obj, &info);
  case PACKAGESXP:
    return unpack_read_package(obj, &info);
  case NAMESPACESXP:
    return unpack_read_namespace(obj, &info);
  case ENVSXP:
    return unpack_read_environment(obj, &info);
    // 3. Symbols
  case SYMSXP:
    return unpack_read_symbol(obj, &info);
    // 4. Dotted pair lists
  case LISTSXP:
  case LANGSXP:
  case CLOSXP:
  case PROMSXP:
  case DOTSXP:
    return unpack_read_pairlist(obj, &info);
    // 5. References
  case EXTPTRSXP:  // +attr
    return unpack_read_extptr(obj, &info);
  case WEAKREFSXP: // +attr
    return unpack_read_weakref(obj, &info);
    // 6. Special functions
  case SPECIALSXP: // +attr
  case BUILTINSXP: // +attr
    return unpack_read_builtin(obj, &info);
    // 7. Single string
  case CHARSXP: // +attr
    return unpack_read_charsxp(obj, &info);
    // 8. Vectors!
  case LGLSXP: // +attr
  case INTSXP: // +attr
    return unpack_read_vector_integer(obj, &info);
  case REALSXP: // +attr
    return unpack_read_vector_real(obj, &info);
  case CPLXSXP: // +attr
    return unpack_read_vector_complex(obj, &info);
  case STRSXP: // +attr
    return unpack_read_vector_character(obj, &info);
  case VECSXP: // +attr
  case EXPRSXP: // +attr
    return unpack_read_vector_generic(obj, &info);
    // 9. Weird shit
  case BCODESXP: // +attr
    return unpack_read_bcode(obj, &info);
  case CLASSREFSXP: // +attr
    Rf_error("Can't unpack class references");
  case GENERICREFSXP: // +attr
    Rf_error("Can't unpack generic function references");
    // 10. More vectors
  case RAWSXP: // +attr
    return unpack_read_vector_raw(obj, &info);
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

void unpack_add_attributes(SEXP s, sexp_info *info, unpack_data *obj) {
  if (info->type != CHARSXP) {
    SETLEVELS(s, info->levels);
  }
  SET_OBJECT(s, info->is_object);
  if (TYPEOF(s) == CHARSXP) {
    if (info->has_attr) {
      obj->stream->depth++;
      unpack_read_item(obj);
      obj->stream->depth--;
    }
  } else {
    obj->stream->depth++;
    SET_ATTRIB(s, info->has_attr ? unpack_read_item(obj) : R_NilValue);
    obj->stream->depth--;
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

// MakeReadRefTable
SEXP init_read_ref(rds_index *index) {
  SEXP ret;
  size_t initial_size =
    index == NULL ? INITIAL_REFREAD_TABLE_SIZE : index->ref_table_count;
  SEXP data = PROTECT(allocVector(VECSXP, initial_size));
  if (index == NULL) {
    SET_TRUELENGTH(data, 0);
    ret = CONS(data, R_NilValue);
  } else {
    SEXP read = PROTECT(allocVector(LGLSXP, initial_size));
    memset(INTEGER(read), 0, initial_size * sizeof(int));
    ret = CONS(data, read);
    UNPROTECT(1);
  }
  UNPROTECT(1);
  return ret;
}

// GetReadRef
SEXP get_read_ref(unpack_data *obj, sexp_info *info, int index) {
  int i;
  SEXP data = CAR(obj->ref_objects);
  if (obj->index == NULL) {
    i = index - 1;
    if (i < 0 || i >= LENGTH(data)) {
      Rf_error("reference index out of range");
    }
  } else {
    size_t true_id = obj->index->index[info->id].refid;
    size_t index = obj->index->index[true_id].refid;
    Rprintf("object %d is reqesting %d resolved as %d\n",
            info->id, index, true_id);
    i = index - 1;
  }
  Rprintf("Retrieving reference %d\n", i);
  SEXP ret = VECTOR_ELT(data, i);
  if (ret == R_NilValue) {
    Rf_error("failure in pulling things out");
  }
  return ret;
}

// AddReadRef
void add_read_ref(unpack_data *obj, SEXP value, sexp_info *info) {
  // used by:
  // - [x] SYMSXP
  // - [ ] PERSISTSXP
  // - [x] PACKAGESXP
  // - [x] NAMESPACESXP
  // - [x] ENVSXP
  // - [x] EXTPTRSXP
  // - [x] WEAKREFSXP
  //
  // TODO: I do not think that the growth bits that I have here are R
  // API; TRUELENGTH and SET_TRUELENGTH - it might be instructive to
  // search github.com/cran for usage.  If it's not, it's not actually
  // a huge deal.
  //
  // TODO: there is more to write here for the case where we have the
  // index, because then the id is different.
  SEXP data = CAR(obj->ref_objects);
  int count;
  if (obj->index == NULL) {
    count = TRUELENGTH(data) + 1;
    if (count >= LENGTH(data)) {
      int len;
      SEXP newdata;

      PROTECT(value);
      len = 2 * count;
      newdata = allocVector(VECSXP, len);
      for (int i = 0; i < LENGTH(data); i++) {
        SET_VECTOR_ELT(newdata, i, VECTOR_ELT(data, i));
      }
      SETCAR(obj->ref_objects, newdata);
      data = newdata;
      UNPROTECT(1);
    }
    SET_TRUELENGTH(data, count);
  } else {
    count = obj->index->index[info->id].refid;
    INTEGER(CDR(obj->ref_objects))[count - 1] = 1;
  }
  Rprintf("Inserting reference %d to %d\n", count - 1, info->id);
  SET_VECTOR_ELT(data, count - 1, value);
}

// From base R: src/main/serialize.c
//
// NOTE: the lastname bit is not done properly here.
static SEXP R_FindNamespace1(SEXP info)
{
  SEXP expr, val, where;
  PROTECT(info);
  where = PROTECT(ScalarString(mkChar("serialized object")));
  SEXP s_getNamespace = install("..getNamespace");
  PROTECT(expr = LCONS(s_getNamespace,
                       LCONS(info, LCONS(where, R_NilValue))));
  val = eval(expr, R_GlobalEnv);
  UNPROTECT(3);
  return val;
}
