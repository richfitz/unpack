#include "extract.h"
#include "unpack.h"
#include "rdsi.h"
#include "util.h"

SEXP r_unpack_extract_plan(SEXP r_rdsi, SEXP r_id) {
  const rds_index_t *index = get_rdsi_index(r_rdsi);

  size_t id = scalar_size(r_id, "id");
  size_t len = index->len;
  if (id > len - 1)  {
    Rf_error("id is out of bounds (%d / %d)", id, index->len - 1);
  }

  bool *seen = (bool*) R_alloc(len, sizeof(bool));
  bool *need = (bool*) R_alloc(len, sizeof(bool));
  memset(seen, 0, len * sizeof(bool));
  memset(need, 0, len * sizeof(bool));

  const sexp_info_t * info = index->objects + id;
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

void unpack_extract_plan(const rds_index_t *index, size_t id,
                         const sexp_info_t *focal,
                         bool *seen, bool *need) {
  const sexp_info_t *info = index->objects + id;
  R_xlen_t end = info->end;
  for (size_t i = id; i < (size_t)index->len - 1; ++i) {
    if (!seen[i]) {
      const sexp_info_t *child = index->objects + i;
      if (child->start_data > end) {
        // only search as far as the end of the focal sexp:
        break;
      }
      seen[i] = true;
      if (child->type == REFSXP) {
        size_t i_ref = child->refid;
        R_xlen_t pos = index->objects[i_ref].start_object;
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
SEXP r_unpack_extract(SEXP r_rdsi, SEXP r_id, SEXP r_reuse_ref) {
  rdsi_t *rdsi = get_rdsi(r_rdsi, true);
  size_t id = scalar_size(r_id, "id");
  bool reuse_ref = scalar_logical(r_reuse_ref, "reuse_ref");

  unpack_data_t *obj = unpack_data_create_rdsi(rdsi);
  if (id > (size_t)obj->index->len - 1)  {
    Rf_error("id is out of bounds (%d / %d)", id, obj->index->len - 1);
  }
  return unpack_extract(obj, id, reuse_ref, r_rdsi);
}

SEXP unpack_extract(unpack_data_t *obj, size_t id, bool reuse_ref,
                    SEXP r_rdsi) {
  size_t len = obj->index->len;
  const sexp_info_t * info = obj->index->objects + id;
  bool *seen = (bool*) R_alloc(len, sizeof(bool));
  bool *need = (bool*) R_alloc(len, sizeof(bool));
  memset(seen, 0, len * sizeof(bool));
  memset(need, 0, len * sizeof(bool));
  unpack_extract_plan(obj->index, id, info, seen, need);

  if (reuse_ref) {
    obj->ref_objects = rdsi_get_refs(r_rdsi);
  }
  bool create_ref_objects = obj->ref_objects == R_NilValue;
  if (create_ref_objects) {
    obj->ref_objects = PROTECT(init_read_ref(obj->index));
  }

  int * done = INTEGER(CDR(obj->ref_objects));
  for (size_t i = 0; i < len; ++i) {
    if (need[i]) {
      size_t j = obj->index->objects[i].refid;
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
    rdsi_set_refs(r_rdsi, obj->ref_objects);
  }
  if (create_ref_objects) {
    UNPROTECT(1);
  }

  return ret;
}

SEXP unpack_extract1(unpack_data_t *obj, size_t id) {
  buffer_move_to(obj->buffer, obj->index->objects[id].start_object);
  obj->count = id;
  return unpack_read_item(obj);
}
