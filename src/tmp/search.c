#include "search.h"
#include "util.h"
#include "index.h"

SEXP r_index_search_attribute(SEXP r_x, SEXP r_index, SEXP r_id, SEXP r_name) {
  unpack_data *obj = unpack_data_prepare(r_x);
  obj->index = get_index(r_index, true);
  size_t id = scalar_size(r_id, "id");
  if (id > (obj->index->id - 1)) {
    Rf_error("id is out of bounds");
  }
  const char *name = scalar_character(r_name, "name");
  return ScalarInteger(index_search_attribute(obj, id, name));
}

SEXP r_index_search_character(SEXP r_x, SEXP r_index, SEXP r_id, SEXP r_str) {
  unpack_data *obj = unpack_data_prepare(r_x);
  obj->index = get_index(r_index, true);
  size_t id = scalar_size(r_id, "id");
  if (id > (obj->index->id - 1)) {
    Rf_error("id is out of bounds");
  }
  const char * str = scalar_character(r_str, "str");
  return ScalarInteger(index_search_character(obj, id, str));
}

int index_search_attribute(unpack_data * obj, size_t id, const char *name) {
  int id_attr = index_find_attributes(obj->index, id);
  if (id_attr == NA_INTEGER) {
    return NA_INTEGER;
  }
  sexp_info *info_attr = obj->index->index + (size_t) id_attr;

  const size_t name_len = strlen(name);
  const char *name_data;
  size_t name_data_len = unpack_write_string(obj, name, name_len, &name_data);

  int ret = NA_INTEGER;
  while (info_attr->type == LISTSXP) {
    size_t id_name_sym =
      index_find_nth_child(obj->index, id_attr, info_attr->has_attr + 1);
    if (obj->index->index[id_name_sym].type == REFSXP) {
      id_name_sym = obj->index->index[id_name_sym].refid;
    }
    if (obj->index->index[id_name_sym].type != SYMSXP) {
      Rf_error("logic failure in in index_search_attribute");
    }
    size_t id_name = index_find_nth_child(obj->index, id_name_sym, 1);

    if (index_compare_charsxp(obj, id_name, name, name_len, name_data_len)) {
      ret = index_find_car(obj->index, id_attr);
      break;
    }
    id_attr = index_find_cdr(obj->index, id_attr);
    info_attr = obj->index->index + id_attr;
  }
  return ret;
}

// NOTE: this returns the *child index* of the string, not the id of
// the string.  The latter is never interesting; we already have the
// string!  We return this in base-1 because that's what nth_child
// works with.
int index_search_character(unpack_data * obj, size_t id, const char *str) {
  sexp_info *info = obj->index->index + id;
  // TODO: I think for consistency this belongs in the r_ method
  if (info->type != STRSXP) {
    Rf_error("index_search_character requires a STRSXP, not a %s",
             type2str(info->type));
  }

  const size_t str_len = strlen(str);
  const char *str_data;
  size_t str_data_len = unpack_write_string(obj, str, str_len, &str_data);

  size_t at = id;
  for (R_xlen_t i = 0; i < info->length; ++i) {
    int j = index_find_next_child(obj->index, id, at);
    // TODO: this can be removed eventually; it should never trigger
    if (j == NA_INTEGER) {
      Rf_error("failed to find child %d", i);
    }
    if (index_compare_charsxp(obj, j, str_data, str_len, str_data_len)) {
      return i + 1;
    }
    at = j;
  }
  return NA_INTEGER;
}

bool index_compare_charsxp(unpack_data * obj, size_t id,
                           const char *str_data, size_t str_length,
                           size_t str_data_length) {
  bool same = false;
  sexp_info *info_char = obj->index->index + id;
  if ((size_t)info_char->length == str_length) {
    // TODO: in the case of ascii, check that start_attr - start_data
    // is the same as str_data_length before doing the memcpy.  We
    // could also just use that length entirely I think.
    const char* str_cmp = stream_at(obj->stream, info_char->start_data);
    same = memcmp(str_cmp, str_data, str_data_length) == 0;
  }
  return same;
}
