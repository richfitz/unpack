#include <R.h>
#include <Rinternals.h>
#include <stdint.h>

void xdr_read_int(const void *src, int32_t *dst);
void xdr_read_double(const void *src, double *dst);
void xdr_read_complex(const void *src, Rcomplex *dst);

void xdr_read_int_vector(const void *src, size_t n, int *dst);
void xdr_read_double_vector(const void *src, size_t n, double *dst);
void xdr_read_complex_vector(const void *src, size_t n, Rcomplex *dst);

int xdr_decode_int(const void *src);
double xdr_decode_double(const void *src);

SEXP r_xdr_read_int(SEXP r_x);
SEXP r_xdr_read_double(SEXP r_x);
SEXP r_xdr_read_complex(SEXP r_x);
