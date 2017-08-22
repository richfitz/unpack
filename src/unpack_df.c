#include "unpack_df.h"
#include "rdsi.h"
#include "search.h"
#include "util.h"
#include "upstream.h"

static void r_unpack_df_finalize(SEXP r_unpack_df);

SEXP r_unpack_df_create(SEXP r_rdsi, SEXP r_id) {
  rdsi_t *rdsi = get_rdsi(r_rdsi, true);
  size_t id = scalar_size(r_id, "id");
  if (id > ((size_t)rdsi->index->len - 1)) {
    Rf_error("id is out of bounds");
  }
  unpack_data_t *obj = unpack_data_create_rdsi(rdsi, true);
  if (!index_search_inherits(obj, id, "data.frame")) {
    Rf_error("is not a data.frame");
  }

  unpack_df_t * df = (unpack_df_t*) Calloc(1, unpack_df_t);
  SEXP ret = PROTECT(R_MakeExternalPtr(rdsi, R_NilValue, r_rdsi));
  R_RegisterCFinalizer(ret, r_unpack_df_finalize);

  df->rdsi = rdsi;
  df->id = id;
  df->obj = obj;
  unpack_df_dim(df);

  UNPROTECT(1);
  return ret;
}

SEXP r_unpack_df_dim(SEXP r_unpack_df) {
  unpack_df_t *unpack_df = get_unpack_df(r_unpack_df, true);
  SEXP r_dim = PROTECT(allocVector(INTSXP, 2));
  memcpy(INTEGER(r_dim), unpack_df->dim, 2 * sizeof(int));
  UNPROTECT(1);
  return r_dim;
}

void unpack_df_dim(unpack_df_t *unpack_df) {
  unpack_data_t *obj = unpack_df->obj;
  const size_t id = unpack_df->id;

  const int id_row_names = index_search_attribute(obj, id, "row.names");
  if (id_row_names == NA_INTEGER) {
    Rf_error("did not find rownames");
  }

  // This duplicates logic in do_shortRowNames, via dim.data.frame
  const sexp_info_t* info = obj->index->objects + id_row_names;
  unpack_df->dim[0] = 0;
  if (info->type == INTSXP && info->length == 2) {
    int * data = (int*)buffer_at(obj->buffer, info->start_data,
                                 2 * sizeof(int));
    unpack_df->dim[0] = abs(data[1]);
  } else if (info->type != NILVALUE_SXP) {
    unpack_df->dim[0] = info->length;
  }
  unpack_df->dim[1] = obj->index->objects[id].length;
}

unpack_df_t * get_unpack_df(SEXP r_unpack_df, bool closed_error) {
  // TODO: check that the rdsi pointer is ok too, and if not then
  // clear all else before throwing
  return (unpack_df_t*) check_extptr_valid(r_unpack_df, "unpack_df",
                                           closed_error);
}

static void r_unpack_df_finalize(SEXP r_unpack_df) {
  unpack_df_t * unpack_df = get_unpack_df(r_unpack_df, false);
  if (unpack_df == NULL) {
    Free(unpack_df->obj);
    Free(unpack_df);
    R_ClearExternalPtr(r_unpack_df);
    R_SetExternalPtrProtected(r_unpack_df, R_NilValue);
  }
}
