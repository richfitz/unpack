#ifndef UNPACK_INDEX_H
#define UNPACK_INDEX_H

#include "unpack.h"
#include "upstream.h"

enum index_positions {
  IDX_ID,
  IDX_TYPE,
  IDX_LEVELS,
  IDX_IS_OBJECT,
  IDX_HAS_ATTR,
  IDX_HAS_TAG,
  IDX_LENGTH,
  IDX_PARENT,
  IDX_START_OBJECT,
  IDX_START_DATA,
  IDX_START_ATTR,
  IDX_END,
};

typedef struct {
  // NOTE: Not using long things here because it complicates export.
  // This needs changing in index_return and index_grow but perhaps
  // nowhere else...
  sexp_info * index;
  size_t id; // id of the *next* object
  size_t len;
} rds_index;

SEXP r_unpack_index(SEXP x);

void index_init(rds_index *index, size_t n);
void index_grow(rds_index *index);
SEXP index_return(rds_index *index);

void index_build(stream_t stream, rds_index *index, size_t parent);

void index_vector(stream_t stream, rds_index *index, size_t element_size,
                  size_t id);
void index_attributes(stream_t stream, rds_index *index, size_t id);
void index_charsxp(stream_t stream, rds_index *index, size_t id);
void index_pairlist(stream_t stream, rds_index *index, size_t id);
void index_vector_generic(stream_t stream, rds_index *index, size_t id);
void index_vector_character(stream_t stream, rds_index *index, size_t id);
void index_symbol(stream_t stream, rds_index *index, size_t id);

#endif
