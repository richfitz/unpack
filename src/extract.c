#include "extract.h"
#include "unpack.h"
#include "util.h"

// NOTE: This needs some care; it's pretty easy to accidenty return an
// internal SEXP type (most probably a CHARSXP) so this should be
// handled nicely.
SEXP r_unpack_extract(SEXP r_x, SEXP r_index, SEXP r_id) {
  rds_index * index = get_index(r_index, true);
  size_t id = scalar_size(r_id, "id");
  if (id > (index->id - 1)) {
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

size_t index_find_id(rds_index * index, int at, size_t start_id) {
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
  return -1;
}

size_t index_find_attributes(rds_index * index, int id) {
  sexp_info *info = index->index + id;
  return info->has_attr ? index_find_id(index, info->start_attr, id) : -1;
}

size_t index_find_car(rds_index * index, int id) {
  sexp_info *info = index->index + id;
  if (info->type != LISTSXP) {
    Rf_error("index_find_car requres a LISTSXP");
  }
  return index_find_nth_daughter(index, id, 1 + info->has_attr + info->has_tag);
}

size_t index_find_cdr(rds_index * index, int id) {
  sexp_info *info = index->index + id;
  if (info->type != LISTSXP) {
    Rf_error("index_find_cdr requres a LISTSXP");
  }
  return index_find_nth_daughter(index, id, 2 + info->has_attr + info->has_tag);
}

int index_find_nth_daughter(rds_index *index, size_t id, size_t n) {
  size_t found = 0;
  size_t i = id;
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
  } while(info->start_object < end);
  return -1;
}

int index_find_attribute(rds_index *index, size_t id, const char *name,
                         stream_t stream) {
  const size_t len = strlen(name);
  size_t id_attr = index_find_attributes(index, id);
  sexp_info *info_attr = index->index + id_attr;
  int ret = -1;
  while (info_attr->type == LISTSXP) {
    // TODO: looks like I am off by one here
    size_t id_name_sym =
      index_find_nth_daughter(index, id_attr, info_attr->has_attr + 1);
    size_t id_name =
      index_find_nth_daughter(index, id_name_sym, 1);
    // better here would be to use read_charsxp with the counter moved
    // along to start_object to avoid reading two bytes
    if (index->index[id_name].length == (R_xlen_t)len) {
      stream_move_to(stream, index->index[id_name].start_object);
      SEXP el = PROTECT(unpack_read_item(stream));
      bool same = strcmp(CHAR(el), name) == 0;
      UNPROTECT(1);
      if (same) {
        ret = index_find_car(index, id_attr);
        break;
      }
    }
    id_attr = index_find_cdr(index, id_attr);
    info_attr = index->index + id_attr;
  }
  return ret;
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
SEXP r_index_find_nth_daughter(SEXP r_index, SEXP r_id, SEXP r_n) {
  rds_index *index = get_index(r_index, true);
  size_t
    id = scalar_size(r_id, "id"),
    n = scalar_size(r_n, "n");
  return ScalarInteger(index_find_nth_daughter(index, id, n));
}

SEXP r_index_find_attribute(SEXP r_index, SEXP r_id, SEXP r_name, SEXP r_x) {
  rds_index * index = get_index(r_index, true);
  size_t id = scalar_size(r_id, "id");
  if (id > (index->id - 1)) {
    Rf_error("id is out of bounds");
  }
  const char * name = scalar_character(r_name, name);
  struct stream_st stream;
  unpack_prepare(r_x, &stream);
  int id_attr = index_find_attribute(index, id, name, &stream);
  return ScalarInteger(id_attr < 0 ? NA_INTEGER : id_attr);
}
