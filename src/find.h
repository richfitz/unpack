#include "unpack.h"

SEXP r_index_find_element(SEXP r_rdsi, SEXP r_id, SEXP r_i);
SEXP r_index_find_nth_child(SEXP r_rdsi, SEXP r_id, SEXP r_n);
SEXP r_index_find_car(SEXP r_rdsi, SEXP r_id);
SEXP r_index_find_cdr(SEXP r_rdsi, SEXP r_id);
SEXP r_index_find_attributes(SEXP r_rdsi, SEXP r_id);
SEXP r_index_find_id(SEXP r_rdsi, SEXP r_at, SEXP r_start_id);

int index_find_element(const rds_index_t *index, size_t id, size_t i);
int index_find_element_list(const rds_index_t *index, size_t id, size_t i);

int index_find_nth_child(const rds_index_t *index, size_t id, size_t n);
int index_find_next_child(const rds_index_t *index, size_t id, size_t at);
int index_find_car(const rds_index_t * index, int id);
int index_find_cdr(const rds_index_t * index, int id);
int index_find_attributes(const rds_index_t * index, int id);
int index_find_id(const rds_index_t * index, int at, size_t start_id);
