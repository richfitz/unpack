#include "extract.h"
#include "unpack.h"
#include "util.h"

SEXP r_unpack_extract_plan(SEXP r_index, SEXP r_id) {
  rds_index *index = get_index(r_index, true);
  size_t id = scalar_size(r_id, "id");
  size_t len = index->id;
  if (id > len - 1)  {
    Rf_error("id is out of bounds (%d / %d)", id, index->id - 1);
  }

  bool *seen = (bool*) R_alloc(len, sizeof(bool));
  bool *need = (bool*) R_alloc(len, sizeof(bool));
  memset(seen, 0, len * sizeof(bool));
  memset(need, 0, len * sizeof(bool));

  sexp_info * info = index->index + id;
  unpack_extract_plan(index, id, info, seen, need);

  SEXP r_seen = PROTECT(allocVector(LGLSXP, len));
  SEXP r_need = PROTECT(allocVector(LGLSXP, len));
  int *c_seen = INTEGER(r_seen), *c_need = INTEGER(r_need);
  for (size_t i = 0; i < len; ++i) {
    c_seen[i] = seen[i];
    c_need[i] = need[i];
  }
  setAttrib(r_need, install("seen"), r_seen);
  UNPROTECT(2);
  return r_need;
}

void unpack_extract_plan(rds_index *index, size_t id, const sexp_info *focal,
                         bool *seen, bool *need) {
  sexp_info *info = index->index + id;
  R_xlen_t end = info->end;
  for (size_t i = id; i < index->id - 1; ++i) {
    if (!seen[i]) {
      sexp_info *child = index->index + i;
      if (child->start_data > end) {
        // only search as far as the end of the focal sexp:
        break;
      }
      seen[i] = true;
      if (child->type == REFSXP) {
        size_t i_ref = child->refid;
        R_xlen_t pos = index->index[i_ref].start_object;
        if (pos < focal->start_object || pos > focal->end) {
          if (pos > focal->end) {
            // I don't think that this is possible because it implies
            // a forward reference.  It might end up being possible
            // though via an environment, but then would already be
            // resolved.  This is related to the comment below about
            // recursion.
            Rf_error("I don't think this is possible");
          }
          Rprintf("we need %d (%d < %d)\n", i_ref, pos, focal->start_object);
          need[i_ref] = true;
          // I don't actually know that this needs to be recursive
          // because these will be resolved in time.  We will loop
          // through these references in order.  So if the first
          // reference needs a second reference, then it should be
          // resolved automatically?  That will not be the case if
          // that reference occurs *before* it though!  Practically,
          // this is only an issue with environments, so hopefully
          // will not be that big a deal!
          if (!seen[i_ref]) {
            Rprintf("recursing...\n");
            unpack_extract_plan(index, i_ref, child, seen, need);
          }
        }
      }
    }
  }
}

SEXP r_unpack_index_refs(SEXP r_index) {
  get_index(r_index, true); // for side effects
  SEXP refs = R_ExternalPtrProtected(r_index);
  return refs == R_NilValue ? R_NilValue : CAR(refs);
}

SEXP r_unpack_index_refs_clear(SEXP r_index) {
  get_index(r_index, true); // for side effects
  R_SetExternalPtrProtected(r_index, R_NilValue);
  return R_NilValue;
}

// NOTE: This needs some care; it's pretty easy to accidenty return an
// internal SEXP type (most probably a CHARSXP) so this should be
// handled nicely.
SEXP r_unpack_extract(SEXP r_x, SEXP r_index, SEXP r_id, SEXP r_reuse_ref) {
  size_t id = scalar_size(r_id, "id");
  bool reuse_ref = scalar_logical(r_reuse_ref, "reuse_ref");

  unpack_data *obj = unpack_data_prepare(r_x);
  obj->index = get_index(r_index, true);

  // TODO: duplication with above, and this is something that will be
  // needed quite a bit.  Probably it all runs together though to the
  // resolution (As unpack_extract_resolve).  Or rewrite to return bool*
  size_t len = obj->index->id;
  if (id > len - 1)  {
    Rf_error("id is out of bounds (%d / %d)", id, obj->index->id - 1);
  }
  sexp_info * info = obj->index->index + id;
  bool *seen = (bool*) R_alloc(len, sizeof(bool));
  bool *need = (bool*) R_alloc(len, sizeof(bool));
  memset(seen, 0, len * sizeof(bool));
  memset(need, 0, len * sizeof(bool));
  unpack_extract_plan(obj->index, id, info, seen, need);

  if (reuse_ref) {
    obj->ref_objects = R_ExternalPtrProtected(r_index);
  }
  bool create_ref_objects = obj->ref_objects == R_NilValue;
  if (create_ref_objects) {
    obj->ref_objects = PROTECT(init_read_ref(obj->index));
  }

  int * done = INTEGER(CDR(obj->ref_objects));
  for (size_t i = 0; i < len; ++i) {
    if (need[i]) {
      size_t j = obj->index->index[i].refid;
      if (done[j - 1] == 1) {
        Rprintf("(unpack_extract) object %d (ref %d) already extracted\n",
                i, j);
      } else {
        Rprintf("(unpack_extract) object %d (ref %d) needs extracting...\n",
                i, j);
        unpack_extract(obj, i);
      }
    }
  }
  SEXP ret = unpack_extract(obj, id);

  if (reuse_ref) {
    R_SetExternalPtrProtected(r_index, obj->ref_objects);
  }
  if (create_ref_objects) {
    UNPROTECT(1);
  }

  return ret;
}

SEXP unpack_extract(unpack_data *obj, size_t id) {
  stream_move_to(obj->stream, obj->index->index[id].start_object);
  obj->count = id;
  return unpack_read_item(obj);
}

// like `[[` - extract *one* element
SEXP r_unpack_extract_element(SEXP r_x, SEXP r_index, SEXP r_id, SEXP r_i,
                              SEXP r_error_if_missing) {
  unpack_data *obj = unpack_data_prepare(r_x);
  obj->index = get_index(r_index, true);
  size_t id = scalar_size(r_id, "id");
  size_t i = scalar_size(r_i, "i");
  if (i == 0) {
    Rf_error("Expected a nonzero positive size for 'i'");
  }
  bool error_if_missing =
    scalar_logical(r_error_if_missing, "error_if_missing");

  return unpack_extract_element(obj, id, i, error_if_missing);
}

SEXP unpack_extract_element(unpack_data *obj, size_t id, size_t i,
                            bool error_if_missing) {
  if (id > (obj->index->id - 1)) {
    Rf_error("id is out of bounds");
  }
  sexp_info * info = obj->index->index + id;
  switch(info->type) {
  case VECSXP:
    return unpack_extract_element_list(obj, id, i, error_if_missing);
  default:
    Rf_error("Cannot extract element from a %s", type2str(info->type));
  }
}

SEXP unpack_extract_element_list(unpack_data *obj, size_t id, size_t i,
                                 bool error_if_missing) {
  int j = index_find_nth_child(obj->index, id, i); // NOTE: naturally base-1
  SEXP ret = R_NilValue;
  if (j < 0) {
    if (error_if_missing) {
      Rf_error("Index %d out of bounds; must be on [1, %d]",
               i, obj->index->index[i].length);
    }
  } else {
    ret = unpack_extract(obj, j);
  }
  return ret;
}

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
  return -1;
}

int index_find_attributes(rds_index * index, int id) {
  sexp_info *info = index->index + id;
  return info->has_attr ? index_find_id(index, info->start_attr, id) : -1;
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
  return -1;
}

int index_find_attribute(unpack_data * obj, size_t id, const char *name) {
  const size_t len = strlen(name);
  size_t id_attr = index_find_attributes(obj->index, id);
  sexp_info *info_attr = obj->index->index + id_attr;
  int ret = -1;
  while (info_attr->type == LISTSXP) {
    size_t id_name_sym =
      index_find_nth_child(obj->index, id_attr, info_attr->has_attr + 1);
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

int index_find_charsxp(unpack_data * obj, size_t id, const char *str) {
  const size_t len = strlen(str);
  sexp_info *info = obj->index->index + id;
  if (info->type != STRSXP) {
    Rf_error("index_find_charsxp requires a STRSXP, not a %s",
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
  return -1;
}

bool index_compare_charsxp(unpack_data * obj, size_t id,
                           const char *str, size_t len) {
  bool same = false;
  sexp_info *info_chr = obj->index->index + id;
  if ((size_t)info_chr->length == len) {
    // TODO: could do this better if we can avoid deoding the
    // character entirely.  To do that *encode* the argument CHARSXP
    // then compare the memory directly.  That should be more
    // efficient in most cases.  But nothing here will change with
    // that change so we can delay it until later.  The same process
    // could be used in find attribute
    stream_move_to(obj->stream, info_chr->start_object);
    // better here would be to use read_charsxp with the counter moved
    // along to start_object to avoid reading two bytes
    SEXP el = PROTECT(unpack_read_item(obj));
    same = strcmp(CHAR(el), str) == 0;
    UNPROTECT(1);
  }
  return same;
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
SEXP r_index_find_nth_child(SEXP r_index, SEXP r_id, SEXP r_n) {
  rds_index *index = get_index(r_index, true);
  size_t
    id = scalar_size(r_id, "id"),
    n = scalar_size(r_n, "n");
  return ScalarInteger(index_find_nth_child(index, id, n));
}

SEXP r_index_find_attribute(SEXP r_x, SEXP r_index, SEXP r_id, SEXP r_name) {
  unpack_data *obj = unpack_data_prepare(r_x);
  obj->index = get_index(r_index, true);
  size_t id = scalar_size(r_id, "id");
  if (id > (obj->index->id - 1)) {
    Rf_error("id is out of bounds");
  }
  const char * name = scalar_character(r_name, "name");
  int id_attr = index_find_attribute(obj, id, name);
  return ScalarInteger(id_attr < 0 ? NA_INTEGER : id_attr);
}

SEXP r_index_find_charsxp(SEXP r_x, SEXP r_index, SEXP r_id, SEXP r_str) {
  unpack_data *obj = unpack_data_prepare(r_x);
  obj->index = get_index(r_index, true);
  size_t id = scalar_size(r_id, "id");
  if (id > (obj->index->id - 1)) {
    Rf_error("id is out of bounds");
  }
  const char * str = scalar_character(r_str, "str");
  int id_attr = index_find_charsxp(obj, id, str);
  return ScalarInteger(id_attr < 0 ? NA_INTEGER : id_attr);
}
