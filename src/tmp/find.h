#include "unpack.h"

SEXP r_index_find_element(SEXP r_index, SEXP r_id, SEXP r_i);
SEXP r_index_find_nth_child(SEXP r_index, SEXP r_id, SEXP r_n);
SEXP r_index_find_car(SEXP r_index, SEXP r_id);
SEXP r_index_find_cdr(SEXP r_index, SEXP r_id);
SEXP r_index_find_attributes(SEXP r_index, SEXP r_id);
SEXP r_index_find_id(SEXP r_index, SEXP r_at, SEXP r_start_id);

int index_find_element(rds_index *index, size_t id, size_t i);
int index_find_element_list(rds_index *index, size_t id, size_t i);

int index_find_nth_child(rds_index *index, size_t id, size_t n);
int index_find_next_child(rds_index *index, size_t id, size_t at);
int index_find_car(rds_index * index, int id);
int index_find_cdr(rds_index * index, int id);
int index_find_attributes(rds_index * index, int id);
int index_find_id(rds_index * index, int at, size_t start_id);
