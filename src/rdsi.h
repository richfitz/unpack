#include "structures.h"

SEXP rdsi_create(const data_t * data, const rds_index_t * index,
                 SEXP r_data);
rdsi_t * get_rdsi(SEXP r_ptr, bool closed_error);
SEXP rdsi_get_data(SEXP r_ptr);
