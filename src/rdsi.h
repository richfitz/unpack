#include "structures.h"

SEXP r_rdsi_build(SEXP r_x);

SEXP r_rdsi_get_index_matrix(SEXP r_ptr);
SEXP r_rdsi_get_data(SEXP r_ptr);
SEXP r_rdsi_get_refs(SEXP r_ptr);
SEXP r_rdsi_del_refs(SEXP r_ptr);

SEXP rdsi_create(SEXP r_data, const rds_index_t * index);
rdsi_t * get_rdsi(SEXP r_ptr, bool closed_error);
