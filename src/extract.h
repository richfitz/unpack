#include "index.h"

SEXP r_unpack_extract(SEXP r_x, SEXP r_index_ptr, SEXP r_id);
size_t index_find_attributes(rds_index * index, int id);
size_t index_find_id(rds_index * index, int at, size_t start_id);
size_t index_find_car(rds_index * index, int id);
size_t index_find_cdr(rds_index * index, int id);
int index_find_nth_daughter(rds_index *index, size_t id, size_t n);
int index_find_attribute(rds_index *index, size_t id, const char *name,
                         stream_t stream);

SEXP r_index_find_id(SEXP r_index, SEXP r_id, SEXP r_start_id);
SEXP r_index_find_attributes(SEXP r_index, SEXP r_id);
SEXP r_index_find_car(SEXP r_index, SEXP r_id);
SEXP r_index_find_cdr(SEXP r_index, SEXP r_id);
SEXP r_index_find_nth_daughter(SEXP r_index, SEXP r_id, SEXP r_n);
SEXP r_index_find_attribute(SEXP r_index, SEXP r_id, SEXP r_name, SEXP r_x);
