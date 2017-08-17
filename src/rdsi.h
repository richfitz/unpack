#include "structures.h"

SEXP rdsi_create(const data_t * data, const rds_index_t * index,
                 SEXP r_data);
rdsi_t * get_rdsi(SEXP r_ptr, bool closed_error);
SEXP rdsi_get_data(SEXP r_ptr);

SEXP r_rdsi_get_index_as_matrix(SEXP r_ptr);
SEXP r_rdsi_get_refs(SEXP r_ptr);
SEXP r_rdsi_clear_refs(SEXP r_ptr);
