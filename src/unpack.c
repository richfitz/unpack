#include "unpack.h"
#include "upstream.h"
#include "helpers.h"
#include "xdr.h"

static SEXP R_FindNamespace1(SEXP info);

// R interface
SEXP r_unpack_all(SEXP r_x) {
  unpack_data_t *obj = unpack_data_create_r(r_x);
  obj->ref_objects = PROTECT(init_read_ref(NULL));
  SEXP ret = PROTECT(unpack_read_item(obj));
  buffer_check_empty(obj->buffer);
  UNPROTECT(2);
  return ret;
}

// ReadItem
SEXP unpack_read_item(unpack_data_t *obj) {
  size_t id = obj->count++;

  // First, if we have an index and if the object is a reference
  // object, have a go at pulling it from the set of resolved objects.
  if (obj->index != NULL) {
    // It would be more efficient to expose the bits that are needed
    // here in the index directly I think.  They're used in a couple
    // of places anyway.  But for now, this is OK and no worse than
    // what happens if we go all the way through the usual resolution.
    const sexp_info_t *info_prev = obj->index->objects + id;
    size_t refid = info_prev->refid;
    if (refid > 0 && info_prev->type != REFSXP) {
      if (INTEGER(CDR(obj->ref_objects))[refid - 1]) {
        Rprintf("(unpack_read_item) object %d (ref %d) already extracted\n",
                info_prev->id, refid);
        R_xlen_t end = info_prev->end;
        buffer_move_to(obj->buffer, end);
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

  sexp_info_t info;
  unpack_flags(buffer_read_integer(obj->buffer), &info);
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
               to_sexptype(info.type, "(UNKNOWN)"));
  }
  Rf_error("noreturn");
}

// InIntegerVec
SEXP unpack_read_vector_integer(unpack_data_t *obj, sexp_info_t *info) {
  info->length = buffer_read_length(obj->buffer);
  SEXP s = PROTECT(allocVector(info->type, info->length));
  buffer_read_int_vector(obj->buffer, info->length, INTEGER(s));
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
}

// InRealVec
SEXP unpack_read_vector_real(unpack_data_t *obj, sexp_info_t *info) {
  info->length = buffer_read_length(obj->buffer);
  SEXP s = PROTECT(allocVector(info->type, info->length));
  buffer_read_double_vector(obj->buffer, info->length, REAL(s));
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
}

// InComplexVec
SEXP unpack_read_vector_complex(unpack_data_t *obj, sexp_info_t *info) {
  info->length = buffer_read_length(obj->buffer);
  SEXP s = PROTECT(allocVector(info->type, info->length));
  buffer_read_complex_vector(obj->buffer, info->length, COMPLEX(s));
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
}

// ...no analog
SEXP unpack_read_vector_raw(unpack_data_t *obj, sexp_info_t *info) {
  info->length = buffer_read_length(obj->buffer);
  SEXP s = PROTECT(allocVector(info->type, info->length));
  buffer_read_bytes(obj->buffer, (size_t)info->length, RAW(s));
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
}

// ...no analog
SEXP unpack_read_vector_character(unpack_data_t *obj, sexp_info_t *info) {
  info->length = buffer_read_length(obj->buffer);
  SEXP s = PROTECT(allocVector(info->type, info->length));
  for (R_xlen_t i = 0; i < info->length; ++i) {
    SET_STRING_ELT(s, i, unpack_read_item(obj));
  }
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
}

// ...no analog
SEXP unpack_read_vector_generic(unpack_data_t *obj, sexp_info_t *info) {
  info->length = buffer_read_length(obj->buffer);
  SEXP s = PROTECT(allocVector(info->type, info->length));
  for (R_xlen_t i = 0; i < info->length; ++i) {
    SET_VECTOR_ELT(s, i, unpack_read_item(obj));
  }
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
}

// ...no analog
SEXP unpack_read_pairlist(unpack_data_t *obj, sexp_info_t *info) {
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
  SET_ATTRIB(s, info->has_attr ? unpack_read_item(obj) : R_NilValue);
  SET_TAG(s, info->has_tag ? unpack_read_item(obj) : R_NilValue);
  SETCAR(s, unpack_read_item(obj));
  SETCDR(s, unpack_read_item(obj));
  UNPROTECT(1);
  return s;
}

// ...no analog
SEXP unpack_read_charsxp(unpack_data_t *obj, sexp_info_t *info) {
  // NOTE: *not* read_length() because limited to 2^32 - 1
  info->length = buffer_read_integer(obj->buffer);
  SEXP s;
  if (info->length == -1) {
    PROTECT(s = NA_STRING);
  } else if (info->length < 1000) {
    int enc = CE_NATIVE;
    char cbuf[info->length + 1];
    buffer_read_string(obj->buffer, info->length, cbuf);
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
    buffer_read_string(obj->buffer, info->length, cbuf);
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
SEXP unpack_read_symbol(unpack_data_t *obj, sexp_info_t *info) {
  SEXP s = PROTECT(unpack_read_item(obj));
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

// InRefIndex
int unpack_read_ref_index(unpack_data_t *obj, sexp_info_t *info) {
  int i = UNPACK_REF_INDEX(info->flags);
  if (i == 0) {
    return buffer_read_integer(obj->buffer);
  } else {
    return i;
  }
}

// ...no analog
SEXP unpack_read_ref(unpack_data_t *obj, sexp_info_t *info) {
  // This one is much easier than insertion because we will just
  // arrange to be able to look the reference up.  Though if we do the
  // resolution recursively that won't happen (but I think that a plan
  // is a better bet).
  int index = unpack_read_ref_index(obj, info);
  return get_read_ref(obj, info, index);
}

SEXP unpack_read_package(unpack_data_t *obj, sexp_info_t *info) {
  SEXP s = PROTECT(unpack_read_persistent_string(obj, info));
  s = R_FindPackageEnv(s);
  UNPROTECT(1);
  add_read_ref(obj, s, info);
  return s;
}

SEXP unpack_read_namespace(unpack_data_t *obj, sexp_info_t *info) {
  SEXP s = PROTECT(unpack_read_persistent_string(obj, info));
  s = R_FindNamespace1(s);
  UNPROTECT(1);
  add_read_ref(obj, s, info);
  return s;
}

// This seems unlikely to be API, though it does all compile
SEXP unpack_read_environment(unpack_data_t *obj, sexp_info_t *info) {
  int locked = buffer_read_integer(obj->buffer);
  SEXP s = PROTECT(allocSExp(ENVSXP));
  add_read_ref(obj, s, info);

  SET_ENCLOS(s, unpack_read_item(obj));
  SET_FRAME(s, unpack_read_item(obj));
  SET_HASHTAB(s, unpack_read_item(obj));
  SET_ATTRIB(s, unpack_read_item(obj));

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

SEXP unpack_read_extptr(unpack_data_t *obj, sexp_info_t *info) {
  SEXP s = PROTECT(allocSExp(info->type));
  add_read_ref(obj, s, info);
  R_SetExternalPtrAddr(s, NULL);
  R_SetExternalPtrProtected(s, unpack_read_item(obj));
  R_SetExternalPtrTag(s, unpack_read_item(obj));
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
}

SEXP unpack_read_weakref(unpack_data_t *obj, sexp_info_t *info) {
  SEXP s = PROTECT(R_MakeWeakRef(R_NilValue, R_NilValue, R_NilValue,
                                 FALSE));
  add_read_ref(obj, s, info);
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
}

// InStringVec - almost the same as the above, but with one extra
// integer read and no attribute check, and type set explicitly
SEXP unpack_read_persistent_string(unpack_data_t *obj, sexp_info_t *info) {
  if (buffer_read_integer(obj->buffer) != 0) {
    // This is an R limitation
    Rf_error("names in persistent strings are not supported yet");
  }
  info->length = buffer_read_length(obj->buffer);
  SEXP s = PROTECT(allocVector(STRSXP, info->length));
  for (R_xlen_t i = 0; i < info->length; ++i) {
    SET_STRING_ELT(s, i, unpack_read_item(obj));
  }
  UNPROTECT(1);
  return s;
}

SEXP unpack_read_persist(unpack_data_t *obj, sexp_info_t *info) {
  // PERSISTSXP
  SEXP s = PROTECT(unpack_read_persistent_string(obj, info));
  Rf_error("unimplemented: unpack_read_persistent");
  // s = PersistentRestore(buffer, s);
  //  if (buffer->InPersistHookFunc == NULL)
  //    error(_("no restore method available"));
  //  return buffer->InPersistHookFunc(s, buffer->InPersistHookData);
  UNPROTECT(1);
  add_read_ref(obj, s, info);
  return s;
}

// There is close to zero chance that this is API
SEXP unpack_read_builtin(unpack_data_t *obj, sexp_info_t *info) {
  /*
  info->length = buffer_read_integer(obj->buffer);
  char cbuf[info->length + 1];
  buffer_read_string(obj->buffer, info->length, cbuf);
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

SEXP unpack_read_bcode(unpack_data_t *obj, sexp_info_t *info) {
  // This is another whole level of hell:
  /*
  info->length = buffer_read_integer(obj->buffer);
  SEXP reps = PROTECT(allocVector(VECSXP, info->length));
  SEXP s = ReadBC1(ref_table, reps, buffer);
  unpack_add_attributes(s, info, obj);
  UNPROTECT(1);
  return s;
  */
  Rf_error("unimplemented: unpack_read_bcode");
  return R_NilValue;
}

// Used in the above
void unpack_add_attributes(SEXP s, sexp_info_t *info, unpack_data_t *obj) {
  if (info->type != CHARSXP) {
    SETLEVELS(s, info->levels);
  }
  SET_OBJECT(s, info->is_object);
  if (TYPEOF(s) == CHARSXP) {
    if (info->has_attr) {
      unpack_read_item(obj);
    }
  } else {
    SET_ATTRIB(s, info->has_attr ? unpack_read_item(obj) : R_NilValue);
  }
}

void unpack_flags(int flags, sexp_info_t *info) {
  info->flags = flags;
  info->type = DECODE_TYPE(flags);
  info->levels = DECODE_LEVELS(flags);
  info->is_object = flags & IS_OBJECT_BIT_MASK ? TRUE : FALSE;
  info->has_attr = flags & HAS_ATTR_BIT_MASK ? TRUE : FALSE;
  info->has_tag = flags & HAS_TAG_BIT_MASK ? TRUE : FALSE;
}

// Prep work
unpack_data_t * unpack_data_create(const data_t * data, R_xlen_t len,
                                   bool persist) {
  unpack_data_t * obj;
  if (persist) {
    obj = (unpack_data_t *)R_alloc(1, sizeof(unpack_data_t));
  } else {
    obj = (unpack_data_t *)Calloc(1, unpack_data_t);
  }
  // These all start with safe values:
  obj->ref_objects = R_NilValue;
  obj->index = NULL;
  obj->count = 0;
  // Add real data
  unpack_prepare(data, len, obj);
  return obj;
}

unpack_data_t * unpack_data_create_r(SEXP r_x) {
  if (TYPEOF(r_x) != RAWSXP) {
    Rf_error("Expected a raw string");
  }
  return unpack_data_create(unpack_target_data(r_x), XLENGTH(r_x), false);
}

const data_t * unpack_target_data(SEXP r_x) {
  // eventually support EXTPTR too.  That will probably require a
  // little more work because we often have structures that are
  // slightly more complex than that.  thor is the use case here, but
  // redux could do similar things; they all return bespoke structures
  // that include data and length.
  return RAW(r_x);
}

// TODO: this is not needed where we have an index - in those cases we
// should use the values there perhaps?
void unpack_prepare(const data_t *data, R_xlen_t len, unpack_data_t *obj) {
  obj->buffer = buffer_create(data, len);
  unpack_check_format(obj);
  unpack_check_version(obj);
}

void unpack_check_format(unpack_data_t *obj) {
  char buf[2];
  buffer_read_bytes(obj->buffer, 2, buf);
  switch(buf[0]) {
  case 'A':
    obj->buffer->format = ASCII;
    break;
  case 'B':
    obj->buffer->format = BINARY;
    break;
  case 'X':
    obj->buffer->format = XDR;
    break;
  case '\n':
    obj->buffer->format = ASCII;
    buffer_read_bytes(obj->buffer, 1, buf);
    break;
  default:
    Rf_error("Unknown input type");
  }
}

void unpack_check_version(unpack_data_t *obj) {
  int version, writer_version, release_version;
  version = buffer_read_integer(obj->buffer);
  // Could just walk the reader along 2 * sizeof(int) bytes instead
  writer_version = buffer_read_integer(obj->buffer);
  release_version = buffer_read_integer(obj->buffer);
  if (version != 2) {
    Rf_error("Cannot read rds files in this format");
  }
}

// References
// MakeReadRefTable
SEXP init_read_ref(const rds_index_t *index) {
  SEXP ret;
  size_t initial_size =
    index == NULL ? INITIAL_REFREAD_TABLE_SIZE : index->n_refs;
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
SEXP get_read_ref(unpack_data_t *obj, sexp_info_t *info, int index) {
  int i;
  SEXP data = CAR(obj->ref_objects);
  if (obj->index == NULL) {
    i = index - 1;
    if (i < 0 || i >= LENGTH(data)) {
      Rf_error("reference index out of range");
    }
  } else {
    size_t true_id = obj->index->objects[info->id].refid;
    size_t index = obj->index->objects[true_id].refid;
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
void add_read_ref(unpack_data_t *obj, SEXP value, sexp_info_t *info) {
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
    count = obj->index->objects[info->id].refid;
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

// This exists as a reverse to buffer_write_string (it may move file
// again).
size_t unpack_write_string(unpack_data_t *obj, const char *s, size_t s_len,
                           const char **value) {
  switch (obj->buffer->format) {
  case BINARY:
  case XDR:
    *value = s;
    return s_len;
  case ASCII:
  default:
    Rf_error("Unimplemented (unpack_write_string)");
    *value = NULL;
    return 0;
  }
}
