#include "index.h"

SEXP r_unpack_extract_plan(SEXP r_index, SEXP r_id);
void unpack_extract_plan(rds_index *index, size_t id, const sexp_info *focal,
                         bool *seen, bool *need);

SEXP r_unpack_extract(SEXP r_x, SEXP r_index, SEXP r_id);
SEXP r_unpack_extract_element(SEXP r_x, SEXP r_index, SEXP r_id, SEXP r_i,
                              SEXP r_error_if_missing);

SEXP unpack_extract(unpack_data *obj, size_t id);
SEXP unpack_extract_element(unpack_data *obj, size_t id, size_t i,
                            bool error_if_missing);
SEXP unpack_extract_element_list(unpack_data *obj, size_t id, size_t i,
                                 bool error_if_missing);

int index_find_attributes(rds_index * index, int id);
int index_find_id(rds_index * index, int at, size_t start_id);
int index_find_car(rds_index * index, int id);
int index_find_cdr(rds_index * index, int id);
int index_find_nth_child(rds_index *index, size_t id, size_t n);
int index_find_attribute(unpack_data * obj, size_t id, const char *name);
int index_find_charsxp(unpack_data * obj, size_t id, const char *str);

bool index_compare_charsxp(unpack_data * obj, size_t id,
                           const char *str, size_t len);

SEXP r_index_find_id(SEXP r_index, SEXP r_id, SEXP r_start_id);
SEXP r_index_find_attributes(SEXP r_index, SEXP r_id);
SEXP r_index_find_car(SEXP r_index, SEXP r_id);
SEXP r_index_find_cdr(SEXP r_index, SEXP r_id);
SEXP r_index_find_nth_child(SEXP r_index, SEXP r_id, SEXP r_n);
SEXP r_index_find_attribute(SEXP r_x, SEXP r_index, SEXP r_id, SEXP r_name);
SEXP r_index_find_charsxp(SEXP r_x, SEXP r_index, SEXP r_id, SEXP r_str);
