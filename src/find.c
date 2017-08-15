#include "find.h"
#include "index.h"
#include "util.h"

// For use with '[['
SEXP r_index_find_element(SEXP r_index, SEXP r_id, SEXP r_i) {
  rds_index *index = get_index(r_index, true);
  size_t id = scalar_size(r_id, "id");
  size_t i = scalar_size(r_i, "i");
  if (i == 0) {
    Rf_error("Expected a nonzero positive size for 'i'");
  }
  return ScalarInteger(index_find_element(index, id, i));
}

SEXP r_index_find_nth_child(SEXP r_index, SEXP r_id, SEXP r_n) {
  rds_index *index = get_index(r_index, true);
  size_t
    id = scalar_size(r_id, "id"),
    n = scalar_size(r_n, "n");
  return ScalarInteger(index_find_nth_child(index, id, n));
}

SEXP r_index_find_car(SEXP r_index, SEXP r_id) {
  rds_index *index = get_index(r_index, true);
  size_t id = scalar_size(r_id, "id");
  return ScalarInteger(index_find_car(index, id));
}

SEXP r_index_find_cdr(SEXP r_index, SEXP r_id) {
  rds_index *index = get_index(r_index, true);
  size_t id = scalar_size(r_id, "id");
  return ScalarInteger(index_find_cdr(index, id));
}

SEXP r_index_find_id(SEXP r_index, SEXP r_at, SEXP r_start_id) {
  rds_index *index = get_index(r_index, true);
  size_t
    at = scalar_size(r_at, "at"),
    start_id = scalar_size(r_start_id, "start_id");
  return ScalarInteger(index_find_id(index, at, start_id));
}

SEXP r_index_find_attributes(SEXP r_index, SEXP r_id) {
  rds_index *index = get_index(r_index, true);
  size_t id = scalar_size(r_id, "id");
  return ScalarInteger(index_find_attributes(index, id));
}

int index_find_element(rds_index *index, size_t id, size_t i) {
  if (id > (index->id - 1)) {
    Rf_error("id is out of bounds");
  }
  sexp_info * info = index->index + id;
  switch(info->type) {
  case VECSXP:
    return index_find_element_list(index, id, i);
  default:
    Rf_error("Cannot index into a %s",
             to_sexptype(info->type, "(UNKNOWN)"));
  }
}

int index_find_element_list(rds_index *index, size_t id, size_t i) {
  return index_find_nth_child(index, id, i); // NOTE: naturally base-1
}

int index_find_nth_child(rds_index *index, size_t id, size_t n) {
  size_t found = 0;
  size_t i = id, last = index->id - 1;
  R_xlen_t end = index->index[i].end;
  sexp_info *info = index->index + id;
  do {
    info++;
    i++;
    if (info->parent == (R_xlen_t)id) {
      found++;
    }
    if (found == n) {
      return i;
    }
  } while(i < last && info->start_object < end);
  return NA_INTEGER;
}

int index_find_car(rds_index * index, int id) {
  sexp_info *info = index->index + id;
  if (info->type != LISTSXP) {
    Rf_error("index_find_car requres a LISTSXP");
  }
  return index_find_nth_child(index, id, 1 + info->has_attr + info->has_tag);
}

int index_find_cdr(rds_index * index, int id) {
  sexp_info *info = index->index + id;
  if (info->type != LISTSXP) {
    Rf_error("index_find_cdr requres a LISTSXP");
  }
  return index_find_nth_child(index, id, 2 + info->has_attr + info->has_tag);
}

int index_find_attributes(rds_index * index, int id) {
  sexp_info *info = index->index + id;
  return info->has_attr ?
    index_find_id(index, info->start_attr, id) :
    NA_INTEGER;
}

// Find the id that starts at 'at' (starting looking from the object start_id)
int index_find_id(rds_index * index, int at, size_t start_id) {
  size_t i = start_id;
  R_xlen_t start_at = index->index[i].start_object,
    end = index->index[i].end;
  do {
    if (start_at == at) {
      return i;
    }
    ++i;
    start_at = index->index[i].start_object;
  } while (start_at < end);
  return NA_INTEGER;
}
