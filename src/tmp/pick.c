#include "pick.h"
#include "util.h"
#include "find.h"
#include "extract.h"

SEXP r_unpack_pick_attributes(SEXP r_x, SEXP r_index, SEXP r_id,
                              SEXP r_reuse_ref) {
  // This section will be duplicated in all below
  unpack_data *obj = unpack_data_prepare(r_x);
  obj->index = get_index(r_index, true);
  size_t id = scalar_size(r_id, "id");
  if (id > (obj->index->id - 1)) {
    Rf_error("id is out of bounds");
  }
  bool reuse_ref = scalar_logical(r_reuse_ref, "reuse_ref");

  int id_attr = index_find_attributes(obj->index, id);
  if (id_attr == NA_INTEGER) {
    return R_NilValue;
  } else {
    return unpack_extract(obj, id_attr, reuse_ref, r_index);
  }
}
