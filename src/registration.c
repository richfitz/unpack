#include "unpack.h"
#include "index.h"
#include "extract.h"
#include <R_ext/Rdynload.h>
#include <Rversion.h>

static const R_CallMethodDef call_methods[] = {
  {"Cunpack_all",                  (DL_FUNC) &r_unpack_all,                1},
  {"Cunpack_inspect",              (DL_FUNC) &r_unpack_inspect,            1},
  {"Cunpack_index",                (DL_FUNC) &r_unpack_index,              2},
  {"Cunpack_index_as_matrix",      (DL_FUNC) &r_unpack_index_as_matrix,    1},
  {"Cunpack_extract",              (DL_FUNC) &r_unpack_extract,            3},
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
