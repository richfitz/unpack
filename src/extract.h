#include "index.h"

SEXP r_unpack_extract_plan(SEXP r_index, SEXP r_id);
void unpack_extract_plan(rds_index *index, size_t id, const sexp_info *focal,
                         bool *seen, bool *need);

SEXP r_unpack_extract(SEXP r_x, SEXP r_index, SEXP r_id, SEXP r_reuse_ref);
SEXP r_unpack_extract_element(SEXP r_x, SEXP r_index, SEXP r_id, SEXP r_i,
                              SEXP r_error_if_missing);

SEXP r_unpack_index_refs(SEXP r_index);
SEXP r_unpack_index_refs_clear(SEXP r_index);

SEXP unpack_extract(unpack_data *obj, size_t id);
