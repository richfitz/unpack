#include <R.h>
#include <Rinternals.h>

SEXP r_unpack_pick_attributes(SEXP r_rdsi, SEXP r_id, SEXP r_reuse_ref);
SEXP r_unpack_pick_attribute(SEXP r_rdsi, SEXP r_name, SEXP r_id,
                             SEXP r_reuse_ref);
SEXP r_unpack_pick_typeof(SEXP r_rdsi, SEXP r_id);
SEXP r_unpack_pick_class(SEXP r_rdsi, SEXP r_id, SEXP r_reuse_ref);
