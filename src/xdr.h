#include <R.h>
#include <Rinternals.h>

void xdr_read_int(void *src, int32_t *dst);
void xdr_read_double(void *src, double *dst);
void xdr_read_complex(void *src, Rcomplex *dst);

void xdr_read_int_vector(void *src, size_t n, int *dst);
void xdr_read_double_vector(void *src, size_t n, double *dst);
void xdr_read_rcomplex_vector(void *src, size_t n, Rcomplex *dst);

int xdr_decode_int(void *src);
double xdr_decode_double(void *src);

SEXP r_xdr_read_int(SEXP r_x);
SEXP r_xdr_read_double(SEXP r_x);
SEXP r_xdr_read_complex(SEXP r_x);
