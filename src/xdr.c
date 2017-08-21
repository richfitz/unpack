#include "xdr.h"
#include <stdint.h>

// The question through all of this is 'is it actually faster to do
// the bit shifting here than reverse copy the bytes with stride 4 or
// 8'

static uint32_t new_ntohl(uint32_t x) {
  return (x << 24) | ((x & 0xff00) << 8) | ((x & 0xff0000) >> 8) | (x >> 24);
}

// Tihis needs to take a *int32_t argument because we write the values
// into this position for the double.
void xdr_read_int(void *src, int32_t *dst) {
  *dst = (int32_t)new_ntohl((uint32_t)(*((int32_t *)(src))));
}

void xdr_read_double(void *src, double *dst) {
  int32_t *lp = (int32_t *)dst;
  xdr_read_int(src, lp + 1);
  xdr_read_int(((int*) src) + 1, lp);
}

void xdr_read_complex(void *src, Rcomplex *dst) {
  dst->r = xdr_decode_double(src);
  dst->i = xdr_decode_double(((double*) src) + 1);
}

void xdr_read_int_vector(void *src, size_t n, int *dst) {
  for (size_t i = 0; i < n; ++i) {
    xdr_read_int((int*)src + i * sizeof(int32_t),
                 dst + i * sizeof(int32_t));
  }
}

void xdr_read_double_vector(void *src, size_t n, double *dst) {
  for (size_t i = 0; i < n; ++i) {
    xdr_read_double((int*)src + i * sizeof(double),
                    dst + i * sizeof(double));
  }
}

void xdr_read_rcomplex_vector(void *src, size_t n, Rcomplex *dst) {
  for (size_t i = 0; i < n; ++i) {
    xdr_read_complex((int*)src + i * sizeof(Rcomplex),
                    dst + i * sizeof(Rcomplex));
  }
}

int xdr_decode_int(void *src) {
  int ret;
  xdr_read_int(src, &ret);
  return ret;
}

double xdr_decode_double(void *src) {
  double ret;
  xdr_read_double(src, &ret);
  return ret;
}

SEXP r_xdr_read_int(SEXP r_x) {
  R_xlen_t len = XLENGTH(r_x);
  if (len % sizeof(int) != 0) {
    Rf_error("Expected a multiple of %d", sizeof(int));
  }
  size_t n = LENGTH(r_x) / sizeof(int);
  SEXP ret = PROTECT(allocVector(INTSXP, n));
  xdr_read_int_vector(RAW(r_x), n, INTEGER(ret));
  UNPROTECT(1);
  return ret;
}

SEXP r_xdr_read_double(SEXP r_x) {
  R_xlen_t len = XLENGTH(r_x);
  if (len % sizeof(double) != 0) {
    Rf_error("Expected a multiple of %d", sizeof(double));
  }
  size_t n = LENGTH(r_x) / sizeof(double);
  SEXP ret = PROTECT(allocVector(REALSXP, n));
  memset(REAL(ret), 0, n * sizeof(double));
  xdr_read_double_vector(RAW(r_x), n, REAL(ret));
  UNPROTECT(1);
  return ret;
}

SEXP r_xdr_read_complex(SEXP r_x) {
  R_xlen_t len = XLENGTH(r_x);
  if (len % sizeof(Rcomplex) != 0) {
    Rf_error("Expected a multiple of %d", sizeof(Rcomplex));
  }
  size_t n = LENGTH(r_x) / sizeof(Rcomplex);
  SEXP ret = PROTECT(allocVector(CPLXSXP, n));
  xdr_read_rcomplex_vector(RAW(r_x), n, COMPLEX(ret));
  UNPROTECT(1);
  return ret;
}
