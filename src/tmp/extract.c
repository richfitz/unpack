#include "extract.h"
#include "unpack.h"
#include "util.h"

static void r_unpack_data_finalize(SEXP r_ptr);

SEXP r_unpack_data_create(SEXP r_x, SEXP r_index) {
  // TODO: this will include a checksum of the data that we'll include
  // in the index.
  //
  // TODO: we'll need to be more generous about what x can be
  r_x = PROTECT(duplicate(r_x));
  unpack_data * obj = (unpack_data *)Calloc(1, unpack_data);
  obj->index = get_index(r_index, true);
  unpack_prepare(r_x, obj);
  obj->count = 0;
  SEXP ref_objects = PROTECT(init_read_ref(obj->index));
  obj->ref_objects = ref_objects;

  SEXP ret = PROTECT(R_MakeExternalPtr(obj, r_x, ref_objects));
  R_RegisterCFinalizer(ret, r_unpack_data_finalize);

  UNPROTECT(3);
  return ret;
}

static void r_unpack_data_finalize(SEXP r_ptr) {
  /*
  unpack_data * obj = get_unpack_data(r_ptr, false);
  if (obj == NULL) {
    Free(obj->ref_table);
    Free(obj->obj);
    Free(obj);
    R_ClearExternalPtr(r_ptr);
  }
  */
}

SEXP r_unpack_extract_plan(SEXP r_index, SEXP r_id) {
  rds_index *index = get_index(r_index, true);
  size_t id = scalar_size(r_id, "id");
  size_t len = index->id;
  if (id > len - 1)  {
    Rf_error("id is out of bounds (%d / %d)", id, index->id - 1);
  }

  bool *seen = (bool*) R_alloc(len, sizeof(bool));
  bool *need = (bool*) R_alloc(len, sizeof(bool));
  memset(seen, 0, len * sizeof(bool));
  memset(need, 0, len * sizeof(bool));

  sexp_info * info = index->index + id;
  unpack_extract_plan(index, id, info, seen, need);

  SEXP r_seen = PROTECT(allocVector(LGLSXP, len));
  SEXP r_need = PROTECT(allocVector(LGLSXP, len));
  int *c_seen = INTEGER(r_seen), *c_need = INTEGER(r_need);
  for (size_t i = 0; i < len; ++i) {
    c_seen[i] = seen[i];
    c_need[i] = need[i];
  }
  setAttrib(r_need, install("seen"), r_seen);
  UNPROTECT(2);
  return r_need;
}

void unpack_extract_plan(rds_index *index, size_t id, const sexp_info *focal,
                         bool *seen, bool *need) {
  sexp_info *info = index->index + id;
  R_xlen_t end = info->end;
  for (size_t i = id; i < index->id - 1; ++i) {
    if (!seen[i]) {
      sexp_info *child = index->index + i;
      if (child->start_data > end) {
        // only search as far as the end of the focal sexp:
        break;
      }
      seen[i] = true;
      if (child->type == REFSXP) {
        size_t i_ref = child->refid;
        R_xlen_t pos = index->index[i_ref].start_object;
        if (pos < focal->start_object || pos > focal->end) {
          if (pos > focal->end) {
            // I don't think that this is possible because it implies
            // a forward reference.  It might end up being possible
            // though via an environment, but then would already be
            // resolved.  This is related to the comment below about
            // recursion.
            Rf_error("I don't think this is possible");
          }
          Rprintf("we need %d (%d < %d)\n", i_ref, pos, focal->start_object);
          need[i_ref] = true;
          // I don't actually know that this needs to be recursive
          // because these will be resolved in time.  We will loop
          // through these references in order.  So if the first
          // reference needs a second reference, then it should be
          // resolved automatically?  That will not be the case if
          // that reference occurs *before* it though!  Practically,
          // this is only an issue with environments, so hopefully
          // will not be that big a deal!
          if (!seen[i_ref]) {
            Rprintf("recursing...\n");
            unpack_extract_plan(index, i_ref, child, seen, need);
          }
        }
      }
    }
  }
}

// NOTE: This needs some care; it's pretty easy to accidenty return an
// internal SEXP type (most probably a CHARSXP) so this should be
// handled nicely.
SEXP r_unpack_extract(SEXP r_x, SEXP r_index, SEXP r_id, SEXP r_reuse_ref) {
  size_t id = scalar_size(r_id, "id");
  bool reuse_ref = scalar_logical(r_reuse_ref, "reuse_ref");
  unpack_data *obj = unpack_data_prepare(r_x);
  obj->index = get_index(r_index, true);
  if (id > obj->index->id - 1)  {
    Rf_error("id is out of bounds (%d / %d)", id, obj->index->id - 1);
  }
  return unpack_extract(obj, id, reuse_ref, r_index);
}

SEXP unpack_extract(unpack_data *obj, size_t id, bool reuse_ref,
                    SEXP r_index) {
  size_t len = obj->index->id;
  sexp_info * info = obj->index->index + id;
  bool *seen = (bool*) R_alloc(len, sizeof(bool));
  bool *need = (bool*) R_alloc(len, sizeof(bool));
  memset(seen, 0, len * sizeof(bool));
  memset(need, 0, len * sizeof(bool));
  unpack_extract_plan(obj->index, id, info, seen, need);

  if (reuse_ref) {
    obj->ref_objects = R_ExternalPtrProtected(r_index);
  }
  bool create_ref_objects = obj->ref_objects == R_NilValue;
  if (create_ref_objects) {
    obj->ref_objects = PROTECT(init_read_ref(obj->index));
  }

  int * done = INTEGER(CDR(obj->ref_objects));
  for (size_t i = 0; i < len; ++i) {
    if (need[i]) {
      size_t j = obj->index->index[i].refid;
      if (done[j - 1] == 1) {
        Rprintf("(unpack_extract) object %d (ref %d) already extracted\n",
                i, j);
      } else {
        Rprintf("(unpack_extract) object %d (ref %d) needs extracting...\n",
                i, j);
        unpack_extract1(obj, i);
      }
    }
  }
  SEXP ret = unpack_extract1(obj, id);

  if (reuse_ref) {
    R_SetExternalPtrProtected(r_index, obj->ref_objects);
  }
  if (create_ref_objects) {
    UNPROTECT(1);
  }

  return ret;
}

SEXP unpack_extract1(unpack_data *obj, size_t id) {
  stream_move_to(obj->stream, obj->index->index[id].start_object);
  obj->count = id;
  return unpack_read_item(obj);
}
