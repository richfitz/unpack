#include "unpack.h"
#include "index.h"
#include "extract.h"
#include "find.h"
#include "search.h"

#include <R_ext/Rdynload.h>
#include <Rversion.h>

static const R_CallMethodDef call_methods[] = {
  // wrapper
  {"Cunpack_all",                  (DL_FUNC) &r_unpack_all,                1},

  // index
  {"Cunpack_index",                (DL_FUNC) &r_unpack_index,              2},
  {"Cunpack_index_as_matrix",      (DL_FUNC) &r_unpack_index_as_matrix,    1},

  // extract plan
  {"Cunpack_extract_plan",         (DL_FUNC) &r_unpack_extract_plan,       2},

  // extract
  {"Cunpack_extract",              (DL_FUNC) &r_unpack_extract,            4},
  {"Cunpack_index_refs",           (DL_FUNC) &r_unpack_index_refs,         1},
  {"Cunpack_index_refs_clear",     (DL_FUNC) &r_unpack_index_refs_clear,   1},

  // find things
  {"Cindex_find_element",          (DL_FUNC) &r_index_find_element,        3},
  {"Cindex_find_nth_child",        (DL_FUNC) &r_index_find_nth_child,      3},
  {"Cindex_find_car",              (DL_FUNC) &r_index_find_car,            2},
  {"Cindex_find_cdr",              (DL_FUNC) &r_index_find_cdr,            2},
  {"Cindex_find_attributes",       (DL_FUNC) &r_index_find_attributes,     2},
  {"Cindex_find_id",               (DL_FUNC) &r_index_find_id,             3},

  // search for things
  {"Cindex_search_attribute",      (DL_FUNC) &r_index_search_attribute,    4},
  {"Cindex_search_character",      (DL_FUNC) &r_index_search_character,    4},

  // more
  {"Csexptypes",                   (DL_FUNC) &r_sexptypes,                 0},
  {"Cto_sexptype",                 (DL_FUNC) &r_to_sexptype,               1},
  {NULL,                           NULL,                                   0}
};

void R_init_unpack(DllInfo *info) {
  R_registerRoutines(info, NULL, call_methods, NULL, NULL);
#if defined(R_VERSION) && R_VERSION >= R_Version(3, 3, 0)
  R_useDynamicSymbols(info, FALSE);
  R_forceSymbols(info, TRUE);
#endif
}
