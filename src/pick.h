#include <R.h>
#include <Rinternals.h>
#include <stdbool.h>
#include "structures.h"

SEXP r_unpack_pick_attributes(SEXP r_rdsi, SEXP r_id, SEXP r_reuse_ref);
SEXP r_unpack_pick_attribute(SEXP r_rdsi, SEXP r_name, SEXP r_id,
                             SEXP r_reuse_ref);
SEXP r_unpack_pick_typeof(SEXP r_rdsi, SEXP r_id);
SEXP r_unpack_pick_class(SEXP r_rdsi, SEXP r_id, SEXP r_reuse_ref);
SEXP r_unpack_pick_length(SEXP r_rdsi, SEXP r_id);
SEXP r_unpack_pick_dim(SEXP r_rdsi, SEXP r_id, SEXP r_reuse_ref);

SEXP unpack_pick_dim(unpack_data_t *obj, size_t id,
                     bool reuse_ref, SEXP r_rdsi);
SEXP unpack_pick_dim_df(unpack_data_t *obj, size_t id);
