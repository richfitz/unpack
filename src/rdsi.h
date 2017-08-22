#include "structures.h"

SEXP r_rdsi_build(SEXP r_x);

SEXP r_rdsi_get_index_matrix(SEXP r_ptr);
SEXP r_rdsi_get_data(SEXP r_ptr);
SEXP r_rdsi_get_refs(SEXP r_ptr);
SEXP r_rdsi_del_refs(SEXP r_ptr);
void   rdsi_set_refs(SEXP r_ptr, SEXP refs);
SEXP   rdsi_get_refs(SEXP r_ptr);

SEXP rdsi_create(SEXP r_data, const rds_index_t * index);
rdsi_t * get_rdsi(SEXP r_ptr, bool closed_error);
const rds_index_t * get_rdsi_index(SEXP r_ptr);

unpack_data_t * unpack_data_create_rdsi(rdsi_t *rdsi, bool persist);
