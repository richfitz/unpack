#include "unpack.h"
#include "index.h"
#include "rdsi.h"
#include "helpers.h"
#include "extract.h"
#include "find.h"
#include "search.h"
#include "pick.h"
#include "xdr.h"
#include "unpack_df.h"

#include <R_ext/Rdynload.h>
#include <Rversion.h>

static const R_CallMethodDef call_methods[] = {
  // wrapper
  {"Cunpack_all",                  (DL_FUNC) &r_unpack_all,                1},

  // index
  {"Crdsi_build",                  (DL_FUNC) &r_rdsi_build,                1},
  {"Crdsi_get_index_matrix",       (DL_FUNC) &r_rdsi_get_index_matrix,     1},
  {"Crdsi_get_data",               (DL_FUNC) &r_rdsi_get_data,             1},
  {"Crdsi_get_refs",               (DL_FUNC) &r_rdsi_get_refs,             1},
  {"Crdsi_del_refs",               (DL_FUNC) &r_rdsi_del_refs,             1},

  // extract
  {"Cunpack_extract_plan",         (DL_FUNC) &r_unpack_extract_plan,       2},
  {"Cunpack_extract",              (DL_FUNC) &r_unpack_extract,            3},

  // find things
  {"Cindex_find_element",          (DL_FUNC) &r_index_find_element,        3},
  {"Cindex_find_nth_child",        (DL_FUNC) &r_index_find_nth_child,      3},
  {"Cindex_find_car",              (DL_FUNC) &r_index_find_car,            2},
  {"Cindex_find_cdr",              (DL_FUNC) &r_index_find_cdr,            2},
  {"Cindex_find_attributes",       (DL_FUNC) &r_index_find_attributes,     2},
  {"Cindex_find_id_linear",        (DL_FUNC) &r_index_find_id_linear,      3},
  {"Cindex_find_id_bisect",        (DL_FUNC) &r_index_find_id_bisect,      3},

  // search for things
  {"Cindex_search_attribute",      (DL_FUNC) &r_index_search_attribute,    3},
  {"Cindex_search_character",      (DL_FUNC) &r_index_search_character,    3},
  {"Cindex_search_inherits",       (DL_FUNC) &r_index_search_inherits,     3},

  // pick things
  {"Cunpack_pick_attributes",      (DL_FUNC) &r_unpack_pick_attributes,    3},
  {"Cunpack_pick_attribute",       (DL_FUNC) &r_unpack_pick_attribute,     4},
  {"Cunpack_pick_typeof",          (DL_FUNC) &r_unpack_pick_typeof,        2},
  {"Cunpack_pick_class",           (DL_FUNC) &r_unpack_pick_class,         3},
  {"Cunpack_pick_length",          (DL_FUNC) &r_unpack_pick_length,        2},
  {"Cunpack_pick_dim",             (DL_FUNC) &r_unpack_pick_dim,           3},

  // more
  {"Csexptypes",                   (DL_FUNC) &r_sexptypes,                 0},
  {"Cto_sexptype",                 (DL_FUNC) &r_to_sexptype,               1},

  // xdr
  {"Cxdr_read_int",                (DL_FUNC) &r_xdr_read_int,              1},
  {"Cxdr_read_double",             (DL_FUNC) &r_xdr_read_double,           1},
  {"Cxdr_read_complex",            (DL_FUNC) &r_xdr_read_complex,          1},

  // df
  {"Cunpack_df_create",            (DL_FUNC) &r_unpack_df_create,          2},

  {NULL,                           NULL,                                   0}
};

void R_init_unpack(DllInfo *info) {
  R_registerRoutines(info, NULL, call_methods, NULL, NULL);
#if defined(R_VERSION) && R_VERSION >= R_Version(3, 3, 0)
  R_useDynamicSymbols(info, FALSE);
  R_forceSymbols(info, TRUE);
#endif
}
