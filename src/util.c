#include "util.h"
bool scalar_logical(SEXP x, const char * name) {
  if (TYPEOF(x) != LGLSXP || length(x) != 1) {
    Rf_error("Expected a scalar logical for '%s'", name);
  }
  int ret = INTEGER(x)[0];
  if (ret == NA_LOGICAL) {
    Rf_error("Expected a non-missing scalar logical for '%s'", name);
  }
  return ret == 1;
}

size_t scalar_size(SEXP x, const char *name) {
  if (TYPEOF(x) != INTSXP || length(x) != 1) {
    Rf_error("Expected a scalar integer for '%s'", name);
  }
  int ret = INTEGER(x)[0];
  if (ret < 0) {
    Rf_error("Expected a positive size for '%s'", name);
  }
  return (size_t)ret;
}
