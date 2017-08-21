#ifndef UNPACK_INDEX_H
#define UNPACK_INDEX_H

#include "unpack.h"
#include "upstream.h"

// There are several modes with creating an index that we might want
// to support:
//
// 1. create a matrix - mostly informational and debugging; does not
//    need to be very convenient.
//
// 2. create a pointer that we pass back for use with index
//    exploration functions; again primarily debugging.
//
// 3. create a combined data + index object ("rdsi")

const rds_index_t * index_build(SEXP r_x);

void index_grow(rds_index_mutable_t *index);

void index_item(unpack_data_t *obj, rds_index_mutable_t *index, size_t parent);

void index_vector(unpack_data_t *obj, rds_index_mutable_t *index,
                  size_t id, size_t element_size);
void index_vector_character(unpack_data_t *obj, rds_index_mutable_t *index,
                            size_t id);
void index_vector_generic(unpack_data_t *obj, rds_index_mutable_t *index,
                          size_t id);
void index_pairlist(unpack_data_t *obj, rds_index_mutable_t *index, size_t id);
void index_charsxp(unpack_data_t *obj, rds_index_mutable_t *index, size_t id);
void index_symbol(unpack_data_t *obj, rds_index_mutable_t *index, size_t id);
void index_ref(unpack_data_t *obj, rds_index_mutable_t *index, size_t id);
void index_package(unpack_data_t *obj, rds_index_mutable_t *index, size_t id);
void index_namespace(unpack_data_t *obj, rds_index_mutable_t *index, size_t id);
void index_environment(unpack_data_t *obj, rds_index_mutable_t *index,
                       size_t id);
void index_extptr(unpack_data_t *obj, rds_index_mutable_t *index, size_t id);
void index_weakref(unpack_data_t *obj, rds_index_mutable_t *index, size_t id);
void index_persistent_string(unpack_data_t *obj, rds_index_mutable_t *index,
                             size_t id);

void index_attributes(unpack_data_t *obj, rds_index_mutable_t *index,
                      size_t id);

SEXP init_read_index_ref();
int get_read_index_ref(unpack_data_t *obj, rds_index_mutable_t *index,
                       int idx);
void add_read_index_ref(unpack_data_t *obj, rds_index_mutable_t *index,
                        size_t id);

SEXP index_as_matrix(const rds_index_t *index);

#endif
