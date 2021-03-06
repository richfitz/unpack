#include "search.h"
#include "util.h"
#include "index.h"
#include "rdsi.h"
#include "helpers.h"

SEXP r_index_search_attribute(SEXP r_rdsi, SEXP r_id, SEXP r_name) {
  rdsi_t *rdsi = get_rdsi(r_rdsi, true);
  size_t id = scalar_size(r_id, "id");
  if (id > ((size_t)rdsi->index->len - 1)) {
    Rf_error("id is out of bounds");
  }
  const char *name = scalar_character(r_name, "name");

  unpack_data_t *obj = unpack_data_create_rdsi(rdsi, false);
  return ScalarInteger(index_search_attribute(obj, id, name));
}

SEXP r_index_search_character(SEXP r_rdsi, SEXP r_id, SEXP r_str) {
  rdsi_t *rdsi = get_rdsi(r_rdsi, true);
  size_t id = scalar_size(r_id, "id");
  if (id > ((size_t)rdsi->index->len - 1)) {
    Rf_error("id is out of bounds");
  }
  const char *str = scalar_character(r_str, "str");

  unpack_data_t *obj = unpack_data_create_rdsi(rdsi, false);
  return ScalarInteger(index_search_character(obj, id, str));
}

SEXP r_index_search_inherits(SEXP r_rdsi, SEXP r_id, SEXP r_what) {
  rdsi_t *rdsi = get_rdsi(r_rdsi, true);
  size_t id = scalar_size(r_id, "id");
  if (id > ((size_t)rdsi->index->len - 1)) {
    Rf_error("id is out of bounds");
  }
  const char * what = scalar_character(r_what, "what");

  unpack_data_t *obj = unpack_data_create_rdsi(rdsi, false);
  return ScalarLogical(index_search_inherits(obj, id, what));
}

int index_search_attribute(unpack_data_t * obj, size_t id, const char *name) {
  int id_attr = index_find_attributes(obj->index, id);
  if (id_attr == NA_INTEGER) {
    return NA_INTEGER;
  }
  const sexp_info_t *info_attr = obj->index->objects + (size_t) id_attr;

  const size_t name_len = strlen(name);
  const char *name_data;
  size_t name_data_len = unpack_write_string(obj, name, name_len, &name_data);

  int ret = NA_INTEGER;
  while (info_attr->type == LISTSXP) {
    size_t id_name_sym =
      index_find_nth_child(obj->index, id_attr, info_attr->has_attr + 1);
    if (obj->index->objects[id_name_sym].type == REFSXP) {
      id_name_sym = obj->index->objects[id_name_sym].refid;
    }
    if (obj->index->objects[id_name_sym].type != SYMSXP) {
      Rf_error("logic failure in in index_search_attribute");
    }
    size_t id_name = index_find_nth_child(obj->index, id_name_sym, 1);

    if (index_compare_charsxp(obj, id_name, name, name_len, name_data_len)) {
      ret = index_find_car(obj->index, id_attr);
      break;
    }
    id_attr = index_find_cdr(obj->index, id_attr);
    info_attr = obj->index->objects + id_attr;
  }
  return ret;
}

// NOTE: this returns the *child index* of the string, not the id of
// the string.  The latter is never interesting; we already have the
// string!  We return this in base-1 because that's what nth_child
// works with.
int index_search_character(unpack_data_t * obj, size_t id, const char *str) {
  const sexp_info_t *info = obj->index->objects + id;
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

// TODO: length/len consistency
bool index_compare_charsxp(unpack_data_t * obj, size_t id,
                           const char *str_data, size_t str_length,
                           size_t str_data_length) {
  bool same = false;
  const sexp_info_t *info_char = obj->index->objects + id;
  if ((size_t)info_char->length == str_length) {
    // TODO: in the case of ascii, check that start_attr - start_data
    // is the same as str_data_length before doing the memcpy.  We
    // could also just use that length entirely I think.
    const data_t* str_cmp = buffer_at(obj->buffer, info_char->start_data,
                                      str_data_length);
    same = memcmp(str_cmp, str_data, str_data_length) == 0;
  }
  return same;
}

bool index_compare_charsxp_str(unpack_data_t * obj, size_t id,
                               const char *str) {
  const size_t str_len = strlen(str);
  const char *str_data;
  const size_t str_data_len = unpack_write_string(obj, str, str_len, &str_data);
  const data_t* str_cmp = buffer_at(obj->buffer,
                                    obj->index->objects[id].start_data,
                                    str_data_len);
  bool same = memcmp(str_cmp, str_data, str_data_len) == 0;
  return same;
}

bool index_search_inherits(unpack_data_t *obj, size_t id, const char *what) {
  int id_attr = index_search_attribute(obj, id, "class");
  if (id_attr == NA_INTEGER) {
    const char *cl = to_typeof(obj->index->objects[id].type, NULL);
    if (cl == NULL) {
      Rf_error("Unknown type");
    }
    return strcmp(cl, what) == 0;
  } else {
    int res = index_search_character(obj, id_attr, what);
    return res != NA_INTEGER;
  }
}
