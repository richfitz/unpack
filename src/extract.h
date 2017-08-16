#include "index.h"

SEXP r_unpack_extract_plan(SEXP r_index, SEXP r_id);
SEXP r_unpack_extract(SEXP r_x, SEXP r_index, SEXP r_id, SEXP r_reuse_ref);

void unpack_extract_plan(rds_index *index, size_t id, const sexp_info *focal,
                         bool *seen, bool *need);
SEXP unpack_extract(unpack_data *obj, size_t id, bool reuse_ref,
                    SEXP r_index);
SEXP unpack_extract1(unpack_data *obj, size_t id);
