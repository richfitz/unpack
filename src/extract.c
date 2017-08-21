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

  const size_t n_refs = index->n_refs;
  bool *seen = (bool*) R_alloc(n_refs, sizeof(bool));
  size_t *need = (size_t*) R_alloc(n_refs, sizeof(size_t));
  memset(seen, 0, n_refs * sizeof(bool));
  memset(need, 0, n_refs * sizeof(size_t));

  const sexp_info_t * info = index->objects + id;
  unpack_extract_plan(index, id, info, seen, need);

  SEXP r_need = PROTECT(allocVector(INTSXP, n_refs));
  int *c_need = INTEGER(r_need);
  for (size_t i = 0; i < n_refs; ++i) {
    c_need[i] = seen[i] ? need[i] : NA_INTEGER;
  }
  UNPROTECT(1);
  return r_need;
}

void unpack_extract_plan(const rds_index_t *index, size_t id,
                         const sexp_info_t *focal,
                         bool *seen, size_t *need) {
  const sexp_info_t *info = index->objects + id;
  R_xlen_t end = info->end;
  for (size_t i = id; i < (size_t)index->len - 1; ++i) {
    ++info;
    if (info->start_data > end) {
      // only search as far as the end of the focal sexp:
      break;
    }
    if (info->type == REFSXP) {
      size_t i_ref = info->refid;
      const sexp_info_t *info_ref = index->objects + i_ref;
      R_xlen_t pos = info_ref->start_object;
      if (pos < focal->start_object || pos > focal->end) {
        if (pos > focal->end) {
          // I don't think that this is possible because it implies
          // a forward reference.  It might end up being possible
          // though via an environment, but then would already be
          // resolved.  This is related to the comment below about
          // recursion.
          Rf_error("I don't think this is possible");
        }
        size_t j = info_ref->refid - 1;
        Rprintf("we need %d (%d) [%d < %d]\n",
                i_ref, j + 1, pos, focal->start_object);
        need[j] = i_ref;
        bool recurse = !seen[j];
        seen[j] = true;
        if (recurse) {
          Rprintf("First time processing this ref: recursing...\n");
          unpack_extract_plan(index, i_ref, info, seen, need);
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
  const size_t n_refs = obj->index->n_refs;
  bool *seen = (bool*) R_alloc(n_refs, sizeof(bool));
  size_t *need = (size_t*) R_alloc(n_refs, sizeof(size_t));
  memset(seen, 0, n_refs * sizeof(bool));
  memset(need, 0, n_refs * sizeof(size_t));

  const sexp_info_t * info = obj->index->objects + id;
  unpack_extract_plan(obj->index, id, info, seen, need);

  if (reuse_ref) {
    obj->ref_objects = rdsi_get_refs(r_rdsi);
  }
  bool create_ref_objects = obj->ref_objects == R_NilValue;
  if (create_ref_objects) {
    obj->ref_objects = PROTECT(init_read_ref(obj->index));
  }

  int * done = INTEGER(CDR(obj->ref_objects));
  for (size_t i = 0; i < n_refs; ++i) {
    if (need[i]) {
      size_t id = need[i];
      if (done[i] == 1) {
        Rprintf("(unpack_extract) object %d (ref %d) already extracted\n",
                id, i);
      } else {
        Rprintf("(unpack_extract) object %d (ref %d) needs extracting...\n",
                id, i);
        unpack_extract1(obj, id);
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
