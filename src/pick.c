#include "pick.h"

#include "util.h"
#include "find.h"
#include "extract.h"
#include "rdsi.h"
#include "search.h"
#include "helpers.h"

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

// This is where I am up to here; it would be good to make use of
// references when doing the search through attributes too, perhaps?
SEXP r_unpack_pick_attribute(SEXP r_rdsi, SEXP r_name, SEXP r_id,
                             SEXP r_reuse_ref) {
  rdsi_t *rdsi = get_rdsi(r_rdsi, true);
  size_t id = scalar_size(r_id, "id");
  if (id > ((size_t)rdsi->index->len - 1)) {
    Rf_error("id is out of bounds");
  }
  bool reuse_ref = scalar_logical(r_reuse_ref, "reuse_ref");
  unpack_data_t *obj = unpack_data_create_rdsi(rdsi);
  const char * name = scalar_character(r_name, "name");

  int id_attr = index_search_attribute(obj, id, name);
  if (id_attr == NA_INTEGER) {
    return R_NilValue;
  } else {
    return unpack_extract(obj, id_attr, reuse_ref, r_rdsi);
  }
}

SEXP r_unpack_pick_typeof(SEXP r_rdsi, SEXP r_id) {
  rdsi_t *rdsi = get_rdsi(r_rdsi, true);
  size_t id = scalar_size(r_id, "id");
  if (id > ((size_t)rdsi->index->len - 1)) {
    Rf_error("id is out of bounds");
  }
  return mkString(to_typeof(rdsi->index->objects[id].type, "UNKNOWN"));
}

SEXP r_unpack_pick_class(SEXP r_rdsi, SEXP r_id, SEXP r_reuse_ref) {
  rdsi_t *rdsi = get_rdsi(r_rdsi, true);
  size_t id = scalar_size(r_id, "id");
  if (id > ((size_t)rdsi->index->len - 1)) {
    Rf_error("id is out of bounds");
  }
  bool reuse_ref = scalar_logical(r_reuse_ref, "reuse_ref");
  unpack_data_t *obj = unpack_data_create_rdsi(rdsi);
  int id_attr = index_search_attribute(obj, id, "class");
  if (id_attr == NA_INTEGER) {
    return mkString(to_typeof(rdsi->index->objects[id].type, "UNKNOWN"));
  } else {
    return unpack_extract(obj, id_attr, reuse_ref, r_rdsi);
  }
}

SEXP r_unpack_pick_length(SEXP r_rdsi, SEXP r_id) {
  rdsi_t *rdsi = get_rdsi(r_rdsi, true);
  size_t id = scalar_size(r_id, "id");
  if (id > ((size_t)rdsi->index->len - 1)) {
    Rf_error("id is out of bounds");
  }
  return ScalarInteger(rdsi->index->objects[id].length);
}

SEXP r_unpack_pick_dim(SEXP r_rdsi, SEXP r_id, SEXP r_reuse_ref) {
  rdsi_t *rdsi = get_rdsi(r_rdsi, true);
  size_t id = scalar_size(r_id, "id");
  if (id > ((size_t)rdsi->index->len - 1)) {
    Rf_error("id is out of bounds");
  }
  bool reuse_ref = scalar_logical(r_reuse_ref, "reuse_ref");
  unpack_data_t *obj = unpack_data_create_rdsi(rdsi);
  return unpack_pick_dim(obj, id, reuse_ref, r_rdsi);
}

SEXP unpack_pick_dim(unpack_data_t *obj, size_t id,
                     bool reuse_ref, SEXP r_rdsi) {
  int id_dim = index_search_attribute(obj, id, "dim");
  if (id_dim != NA_INTEGER) {
    return unpack_extract(obj, id_dim, reuse_ref, r_rdsi);
  } else {
    // Special fallback here for data.frame?  Might be worth making
    // this more extensible somehow.
    int id_class = index_search_attribute(obj, id, "class");
    // NOTE: id + 1 to get to the charsxp; should probably use
    //   find_nth_child(obj->index, id_class, 1);
    if (id_class != NA_INTEGER &&
        index_compare_charsxp_str(obj, id_class + 1, "data.frame")) {
      return unpack_pick_dim_df(obj, id);
    } else {
      return R_NilValue;
    }
  }
}

SEXP unpack_pick_dim_df(unpack_data_t *obj, size_t id) {
  const int id_row_names = index_search_attribute(obj, id, "row.names");
  if (id_row_names == NA_INTEGER) {
    Rf_error("did not find rownames");
  }

  SEXP r_dim = PROTECT(allocVector(INTSXP, 2));
  int *dim = INTEGER(r_dim);

  // This duplicates logic in do_shortRowNames, via dim.data.frame
  const sexp_info_t* info = obj->index->objects + id_row_names;
  dim[0] = 0;
  if (info->type == INTSXP && info->length == 2) {
    int * data = (int*)buffer_at(obj->buffer, info->start_data);
    dim[0] = abs(data[1]);
  } else if (info->type != NILVALUE_SXP) {
    dim[0] = info->length;
  }
  dim[1] = obj->index->objects[id].length;
  UNPROTECT(1);

  return r_dim;
}
