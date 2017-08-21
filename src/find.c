#include "find.h"
#include "index.h"
#include "util.h"
#include "rdsi.h"
#include "helpers.h"

// For use with '[['
SEXP r_index_find_element(SEXP r_rdsi, SEXP r_id, SEXP r_i) {
  const rds_index_t *index = get_rdsi_index(r_rdsi);
  size_t id = scalar_size(r_id, "id");
  size_t i = scalar_size(r_i, "i");
  if (i == 0) {
    Rf_error("Expected a nonzero positive size for 'i'");
  }
  return ScalarInteger(index_find_element(index, id, i));
}

SEXP r_index_find_nth_child(SEXP r_rdsi, SEXP r_id, SEXP r_n) {
  const rds_index_t *index = get_rdsi_index(r_rdsi);
  size_t
    id = scalar_size(r_id, "id"),
    n = scalar_size(r_n, "n");
  return ScalarInteger(index_find_nth_child(index, id, n));
}

SEXP r_index_find_car(SEXP r_rdsi, SEXP r_id) {
  const rds_index_t *index = get_rdsi_index(r_rdsi);
  size_t id = scalar_size(r_id, "id");
  return ScalarInteger(index_find_car(index, id));
}

SEXP r_index_find_cdr(SEXP r_rdsi, SEXP r_id) {
  const rds_index_t *index = get_rdsi_index(r_rdsi);
  size_t id = scalar_size(r_id, "id");
  return ScalarInteger(index_find_cdr(index, id));
}

SEXP r_index_find_id(SEXP r_rdsi, SEXP r_at, SEXP r_start_id) {
  const rds_index_t *index = get_rdsi_index(r_rdsi);
  size_t
    at = scalar_size(r_at, "at"),
    start_id = scalar_size(r_start_id, "start_id");
  return ScalarInteger(index_find_id(index, at, start_id));
}

SEXP r_index_find_attributes(SEXP r_rdsi, SEXP r_id) {
  const rds_index_t *index = get_rdsi_index(r_rdsi);
  size_t id = scalar_size(r_id, "id");
  return ScalarInteger(index_find_attributes(index, id));
}

int index_find_element(const rds_index_t *index, size_t id, size_t i) {
  if (id > ((size_t)index->len - 1)) {
    Rf_error("id is out of bounds");
  }
  const sexp_info_t * info = index->objects + id;
  switch(info->type) {
  case VECSXP:
    return index_find_element_list(index, id, i);
  default:
    Rf_error("Cannot index into a %s",
             to_sexptype(info->type, "(UNKNOWN)"));
  }
}

int index_find_element_list(const rds_index_t *index, size_t id, size_t i) {
  return index_find_nth_child(index, id, i); // NOTE: naturally base-1
}

// NOTE: This is base-1 index!
int index_find_nth_child(const rds_index_t *index, size_t id, size_t n) {
  size_t at = id;
  for (size_t i = 0; i < n; ++i) {
    int j = index_find_next_child(index, id, at);
    if (j == NA_INTEGER) {
      return NA_INTEGER;
    }
    at = j;
  }
  return at;
}

int index_find_next_child(const rds_index_t *index, size_t id, size_t at) {
  R_xlen_t last = index->len - 1;
  R_xlen_t end = index->objects[id].end;
  const sexp_info_t *info = index->objects + at;
  do {
    info++;
    if (info->parent == (R_xlen_t)id) {
      return info->id;
    }
  } while(info->id < last && info->start_object < end);
  return NA_INTEGER;
}

int index_find_car(const rds_index_t * index, int id) {
  const sexp_info_t *info = index->objects + id;
  if (info->type != LISTSXP) {
    Rf_error("index_find_car requres a LISTSXP");
  }
  return index_find_nth_child(index, id, 1 + info->has_attr + info->has_tag);
}

int index_find_cdr(const rds_index_t * index, int id) {
  const sexp_info_t *info = index->objects + id;
  if (info->type != LISTSXP) {
    Rf_error("index_find_cdr requres a LISTSXP");
  }
  return index_find_nth_child(index, id, 2 + info->has_attr + info->has_tag);
}

int index_find_attributes(const rds_index_t * index, int id) {
  const sexp_info_t *info = index->objects + id;
  return info->has_attr ?
    index_find_id(index, info->start_attr, id) :
    NA_INTEGER;
}

// Find the id that starts at 'at' (starting looking from the object start_id)
int index_find_id(const rds_index_t * index, int at, size_t start_id) {
  size_t i = start_id;
  R_xlen_t start_at = index->objects[i].start_object,
    end = index->objects[i].end;
  do {
    if (start_at == at) {
      return i;
    }
    ++i;
    start_at = index->objects[i].start_object;
  } while (start_at < end);
  return NA_INTEGER;
}
