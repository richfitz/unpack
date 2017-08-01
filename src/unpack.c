#include "unpack.h"

SEXP r_unpack_all(SEXP x) {
  if (TYPEOF(x) != RAWSXP) {
    Rf_error("Expected a raw string");
  }
  return R_NilValue;
}
