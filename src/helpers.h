#include <R.h>
#include <Rinternals.h>

SEXP r_sexptypes();
SEXP r_to_sexptype(SEXP x);
const char* to_sexptype(int type, const char * unknown);
