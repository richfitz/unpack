#include "pick.h"

#include "util.h"
#include "find.h"
#include "extract.h"
#include "rdsi.h"

SEXP r_unpack_pick_attributes(SEXP r_rdsi, SEXP r_id, SEXP r_reuse_ref) {
  // This section will be duplicated throughout below, but we need
  // three objects so it's not like this can be done that easily.
  // Abstracting the range check bit would be nice.
  rdsi_t *rdsi = get_rdsi(r_rdsi, true);
  size_t id = scalar_size(r_id, "id");
  if (id > ((size_t)rdsi->index->len - 1)) {
    Rf_error("id is out of bounds");
  }
  bool reuse_ref = scalar_logical(r_reuse_ref, "reuse_ref");
  unpack_data_t *obj = unpack_data_create_rdsi(rdsi);

  int id_attr = index_find_attributes(obj->index, id);
  if (id_attr == NA_INTEGER) {
    return R_NilValue;
  } else {
    return unpack_extract(obj, id_attr, reuse_ref, r_rdsi);
  }
}
