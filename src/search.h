#include "find.h"

SEXP r_index_search_attribute(SEXP r_rdsi, SEXP r_id, SEXP r_name);
SEXP r_index_search_character(SEXP r_rdsi, SEXP r_id, SEXP r_str);
int index_search_attribute(unpack_data_t * obj, size_t id, const char *name);
int index_search_character(unpack_data_t * obj, size_t id, const char *str);
bool index_compare_charsxp(unpack_data_t * obj, size_t id,
                           const char *str_data, size_t str_length,
                           size_t str_data_length);
