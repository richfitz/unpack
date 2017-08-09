#ifndef UNPACK_INDEX_H
#define UNPACK_INDEX_H

#include "unpack.h"
#include "upstream.h"

SEXP r_unpack_index(SEXP x, SEXP r_as_ptr);
SEXP r_unpack_index_as_matrix(SEXP r_ptr);

rds_index * get_index(SEXP r_ptr, bool closed_error);

void index_init(rds_index *index, size_t n);
void index_grow(rds_index *index);
SEXP index_return(rds_index *index);

void index_build(unpack_data *obj, size_t parent);

void index_vector(unpack_data *obj, size_t id, size_t element_size);
void index_attributes(unpack_data *obj, size_t id);
void index_charsxp(unpack_data *obj, size_t id);
void index_ref(unpack_data *obj, size_t id);
void index_pairlist(unpack_data *obj, size_t id);
void index_vector_generic(unpack_data *obj, size_t id);
void index_vector_character(unpack_data *obj, size_t id);
void index_symbol(unpack_data *obj, size_t id);

void init_read_index_ref(unpack_data *obj);
int get_read_index_ref(unpack_data *obj, int idx);
void add_read_index_ref(unpack_data *obj, size_t value);

#endif
