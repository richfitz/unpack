#include "extract.h"
#include "unpack.h"
#include "util.h"

SEXP r_unpack_extract(SEXP r_x, SEXP r_index, SEXP r_id) {
  rds_index * index = (rds_index*)R_ExternalPtrAddr(r_index);
  if (index == NULL) {
    Rf_error("index has been freed; can't use!");
  }
  size_t id = scalar_size(r_id, "id");
  if (id > (index->id + 1)) {
    Rf_error("id is out of bounds");
  }

  struct stream_st stream;
  unpack_prepare(r_x, &stream);

  if (id > 0) {
    sexp_info *info = index->index + id;
    stream_move_to(&stream, info->start_object);
  }

  return unpack_read_item(&stream);
}
