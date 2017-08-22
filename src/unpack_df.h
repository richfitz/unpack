#ifndef UNPACK_UNPACK_DF_H
#define UNPACK_UNPACK_DF_H

#include "structures.h"

typedef struct {
  rdsi_t *rdsi;
  unpack_data_t *obj;
  size_t id;
  int dim[2];
} unpack_df_t;

SEXP r_unpack_df_create(SEXP r_rdsi, SEXP r_id);
void unpack_df_dim(unpack_df_t *df);
unpack_df_t * get_unpack_df(SEXP r_unpack_df, bool closed_error);

#endif
