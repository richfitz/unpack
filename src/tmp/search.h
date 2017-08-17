#include "find.h"

SEXP r_index_search_attribute(SEXP r_x, SEXP r_index, SEXP r_id, SEXP r_name);
SEXP r_index_search_character(SEXP r_x, SEXP r_index, SEXP r_id, SEXP r_str);
int index_search_attribute(unpack_data * obj, size_t id, const char *name);
int index_search_character(unpack_data * obj, size_t id, const char *str);
bool index_compare_charsxp(unpack_data * obj, size_t id,
                           const char *str_data, size_t str_length,
                           size_t str_data_length);
