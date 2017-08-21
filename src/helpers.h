#include <R.h>
#include <Rinternals.h>

SEXP r_sexptypes();
SEXP r_to_sexptype(SEXP x);
SEXP r_to_typeof(SEXP x);

const char* to_sexptype(int type, const char * unknown);
const char* to_typeof(int type, const char * unknown);
