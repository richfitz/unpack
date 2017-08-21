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

const char * scalar_character(SEXP x, const char * name) {
  assert_scalar_character(x, name);
  return CHAR(STRING_ELT(x, 0));
}

void assert_scalar_character(SEXP x, const char *name) {
  if (TYPEOF(x) != STRSXP || length(x) != 1) {
    Rf_error("Expected a scalar character for '%s'", name);
  }
}

bool same_string(const char *a, const char *b, size_t len_a, size_t len_b) {
  return len_a == len_b && memcmp(a, b, len_a) == 0;
}

void * check_extptr_valid(SEXP r_ptr, const char * name, bool closed_error) {
  if (TYPEOF(r_ptr) != EXTPTRSXP) {
    Rf_error("Expected an external pointer for '%s'", name);
  }
  void * data = R_ExternalPtrAddr(r_ptr);
  if (closed_error && data == NULL) {
    Rf_error("'%s' has been freed; can't use!", name);
  }
  return data;
}
