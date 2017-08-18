#include "rdsi.h"
#include "index.h"

static void r_rdsi_finalize(SEXP r_ptr);

SEXP r_rdsi_build(SEXP r_x) {
  SEXP r_index = PROTECT(r_index_build(r_x));
  rds_index_t * index = get_index(r_index, true);
  SEXP ret = rdsi_create(r_x, index);
  UNPROTECT(1);
  return ret;
}

SEXP r_rdsi_get_index_matrix(SEXP r_ptr) {
  rdsi_t *rdsi = get_rdsi(r_ptr, true);
  return index_as_matrix(rdsi->index);
}

SEXP r_rdsi_get_data(SEXP r_ptr) {
  rdsi_t *rdsi = get_rdsi(r_ptr, true);
  SEXP ret = PROTECT(allocVector(RAWSXP, rdsi->data_len));
  memcpy(RAW(ret), rdsi->data, rdsi->data_len);
  UNPROTECT(1);
  return ret;
}

SEXP r_rdsi_get_refs(SEXP r_ptr) {
  get_rdsi(r_ptr, true); // for side effects
  SEXP refs = CDR(R_ExternalPtrProtected(r_ptr));
  return refs == R_NilValue ? R_NilValue : CAR(refs);
}

SEXP r_rdsi_del_refs(SEXP r_ptr) {
  get_index(r_ptr, true); // for side effects
  SEXP prot = R_ExternalPtrProtected(r_ptr);
  SETCDR(prot, R_NilValue);
  R_SetExternalPtrProtected(r_ptr, prot);
  return R_NilValue;
}

// The r_data argument here will be used to ensure that the underlying
// data does not get gc-d; so when using this function pass in the
// object that 'data' come from.  This interface might change, but
// this keeps things fairly extensible I think.
//
// For use with thor we might want to do something with r_data that
// requires that it be checked each time.  Using an
// automatically-invalidating pointer would work.
//
// One option would probably be a weak reference, so that we don't
// prevent gc of the object.
SEXP rdsi_create(SEXP r_data, const rds_index_t * index) {
  rdsi_t * rdsi = (rdsi_t*)Calloc(1, rdsi_t);
  rdsi->data = unpack_target_data(r_data);
  rdsi->index = index;
  rdsi->ref_objects = R_NilValue;
  SEXP prot = PROTECT(CONS(r_data, R_NilValue));
  SEXP ret = PROTECT(R_MakeExternalPtr(rdsi, R_NilValue, prot));
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
