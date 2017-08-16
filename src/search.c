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
  const char * name = scalar_character(r_name, "name");
  return ScalarInteger(index_search_attribute(obj, id, name));
}

SEXP r_index_search_charsxp(SEXP r_x, SEXP r_index, SEXP r_id, SEXP r_str) {
  unpack_data *obj = unpack_data_prepare(r_x);
  obj->index = get_index(r_index, true);
  size_t id = scalar_size(r_id, "id");
  if (id > (obj->index->id - 1)) {
    Rf_error("id is out of bounds");
  }
  const char * str = scalar_character(r_str, "str");
  return ScalarInteger(index_search_charsxp(obj, id, str));
}

int index_search_attribute(unpack_data * obj, size_t id, const char *name) {
  const size_t len = strlen(name);
  size_t id_attr = index_find_attributes(obj->index, id);
  sexp_info *info_attr = obj->index->index + id_attr;
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
    // At this point we could use the reference cache if it's
    // available.  This interacts with the optimisation in
    // index_compare_charsxp, but if a reference is available, then we
    // can just look it up and do a memcmp on the resulting symbol, I
    // think.
    size_t id_name =
      index_find_nth_child(obj->index, id_name_sym, 1);

    if (index_compare_charsxp(obj, id_name, name, len)) {
      ret = index_find_car(obj->index, id_attr);
      break;
    }
    id_attr = index_find_cdr(obj->index, id_attr);
    info_attr = obj->index->index + id_attr;
  }
  return ret;
}

int index_search_charsxp(unpack_data * obj, size_t id, const char *str) {
  const size_t len = strlen(str);
  sexp_info *info = obj->index->index + id;
  if (info->type != STRSXP) {
    Rf_error("index_search_charsxp requires a STRSXP, not a %s",
             type2str(info->type));
  }
  for (R_xlen_t i = 0; i < info->length; ++i) {
    // this could be done more efficiently than this; this will slow
    // down with very long lists and we could pick up from the current
    // point.  This requires a index_find_nth_child(index, id, n, j, i)
    // that will kick start the search from the current spot.  Or,
    // probably simpler, swap this out for something that embeds the
    // main loop from index_find_nth_child within.
    int j = index_find_nth_child(obj->index, id, i);
    if (index_compare_charsxp(obj, j, str, len)) {
      return i;
    }
  }
  return NA_INTEGER;
}

bool index_compare_charsxp(unpack_data * obj, size_t id,
                           const char *str, size_t str_length) {
  bool same = false;
  sexp_info *info_char = obj->index->index + id;
  if ((size_t)info_char->length == str_length) {
    // TODO: could do this *much* better if we can avoid unpacking the
    // character entirely.
    //
    // To do that, we need implement enough of the serialise code
    // (OutString) to create a string encoded as if it was serialised.
    // We pass this in as const char *str (different to CHAR(str)) and
    // also pass in two lengths - one being the length of the string,
    // the other the length of the buffer.  Then it's just a memcmp!
    stream_move_to(obj->stream, info_char->start_object);
    // better here would be to use read_charsxp with the counter moved
    // along to start_object to avoid reading two bytes
    SEXP el = PROTECT(unpack_read_item(obj));
    same = strcmp(CHAR(el), str) == 0;
    UNPROTECT(1);
  }
  return same;
}
