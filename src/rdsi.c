#include "rdsi.h"

static void r_rdsi_finalize(SEXP r_ptr);

// The r_data argument here will be used to ensure that the underlying
// data does not get gc-d; so when using this function pass in the
// object that 'data' come from.  This interface might change, but
// this keeps things fairly extensible I think.
//
// For use with thor we might want to do something with r_data that
// requires that it be checked each time.  Using an
// automatically-invalidating pointer would work.
SEXP rdsi_create(const data_t * data, const rds_index_t * index,
                 SEXP r_data) {
  rdsi_t * rdsi = (rdsi_t*)Calloc(1, rdsi_t);
  rdsi->data = data;
  rdsi->index = index;
  rdsi->ref_objects = R_NilValue;
  SEXP ret = PROTECT(R_MakeExternalPtr(rdsi, R_NilValue, r_data));
  R_RegisterCFinalizer(ret, r_rdsi_finalize);
  return ret;
}

rdsi_t * get_rdsi(SEXP r_ptr, bool closed_error) {
  if (TYPEOF(r_ptr) != EXTPTRSXP) {
    Rf_error("Expected an external pointer for 'index'");
  }
  rdsi_t * ret = (rdsi_t*)R_ExternalPtrAddr(r_ptr);
  if (closed_error && ret == NULL) {
    Rf_error("object has been freed; can't use!");
  }
  return ret;
}

static void r_rdsi_finalize(SEXP r_ptr) {
  rdsi_t * rdsi = get_rdsi(r_ptr, false);
  if (rdsi == NULL) {
    Free(rdsi);
    R_ClearExternalPtr(r_ptr);
    R_SetExternalPtrProtected(r_ptr, R_NilValue);
  }
}

SEXP rdsi_get_data(SEXP r_ptr) {
  rdsi_t *rdsi = get_rdsi(r_ptr, true);
  SEXP ret = PROTECT(allocVector(RAWSXP, rdsi->data_len));
  memcpy(RAW(ret), rdsi->data, rdsi->data_len);
  UNPROTECT(1);
  return ret;
}

SEXP rdsi_get_index_as_matrix(SEXP r_ptr) {
  // rdsi_t *rdsi = get_rdsi(r_ptr, true);
  // return r_unpack_index_as_matrix(SEXP r_ptr);
  return R_NilValue;
}
