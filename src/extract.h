#include "index.h"

SEXP r_unpack_extract_plan(SEXP r_rdsi, SEXP r_id);
SEXP r_unpack_extract(SEXP r_rdsi, SEXP r_id, SEXP r_reuse_ref);

void unpack_extract_plan(const rds_index_t *index, size_t id,
                         const sexp_info_t *focal,
                         bool *seen, size_t *need);
SEXP unpack_extract(unpack_data_t *obj, size_t id, bool reuse_ref,
                    SEXP r_rdsi);
SEXP unpack_extract1(unpack_data_t *obj, size_t id);
