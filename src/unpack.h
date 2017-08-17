#ifndef UNPACK_UNPACK_H
#define UNPACK_UNPACK_H

#include <R.h>
#include <Rinternals.h>
#include <stdbool.h>

// The stream
typedef enum {
  ASCII,
  BINARY,
  XDR
} serialisation_format;

typedef struct sexp_info {
  int flags;
  SEXPTYPE type;
  int levels;
  // NOTE: These are here for the index; they're wasted space for the
  // extraction and I might remove them, but am not sure.  They would
  // be better in a second structure that we keep around just for the
  // indexing, I think...
  R_xlen_t id;
  R_xlen_t length;
  R_xlen_t start_object;
  R_xlen_t start_data;
  R_xlen_t start_attr;
  R_xlen_t end;
  R_xlen_t parent;
  R_xlen_t refid; // Either a from or a to
  // These come from `flags` via a bitmask (actually so do type and
  // levels).  They'll be stored as single bits
  bool is_object;
  bool has_attr;
  bool has_tag;
} sexp_info;

// NOTE: Using R's long size types here in order to support
// unserializing large objects.  We don't support resizing the buffer
// because we're just going to try and read from this rather than
// allocate it.  See R-ints for what the right thing to do here is.
typedef struct stream {
  R_xlen_t size;
  R_xlen_t count;
  unsigned char *buf;
  serialisation_format format;
  int depth;
} stream_t;

// TODO: this needs splitting into two things; an index *builder* and
// an *index*.  The latter is simply the sexp_info* and the length
//
// The former is all the other crap




typedef struct {
  // NOTE: Not using long things here because it complicates export.
  // This needs changing in index_return and index_grow but perhaps
  // nowhere else...
  sexp_info * index;
  size_t id; // id of the *next* object
  size_t len; // capacity
  // This is basically all reference table stuff, and will not be
  // present on index *use*, except for ref_table_count
  size_t ref_table_count;
  size_t ref_table_len;
  size_t *ref_table;
} rds_index;

typedef struct unpack_data {
  stream_t * stream;
  SEXP ref_objects;
  rds_index *index;
  R_xlen_t count;
} unpack_data;

int unpack_read_char(unpack_data *obj);
int unpack_read_integer(unpack_data *obj);
int unpack_read_ref_index(unpack_data *obj, sexp_info *info);
void unpack_read_string(unpack_data *obj, char *buf, int length);

R_xlen_t unpack_read_length(unpack_data *obj);

void unpack_read_bytes(unpack_data *obj, void *buf, R_xlen_t len);

SEXP unpack_read_vector_integer(unpack_data *obj, sexp_info *info);
SEXP unpack_read_vector_real(unpack_data *obj, sexp_info *info);
SEXP unpack_read_vector_complex(unpack_data *obj, sexp_info *info);
SEXP unpack_read_vector_raw(unpack_data *obj, sexp_info *info);
SEXP unpack_read_vector_character(unpack_data *obj, sexp_info *info);
SEXP unpack_read_vector_generic(unpack_data *obj, sexp_info *info);
SEXP unpack_read_charsxp(unpack_data *obj, sexp_info *info);
SEXP unpack_read_ref(unpack_data *obj, sexp_info *info);
SEXP unpack_read_persist(unpack_data *obj, sexp_info *info);
SEXP unpack_read_package(unpack_data *obj, sexp_info *info);
SEXP unpack_read_namespace(unpack_data *obj, sexp_info *info);
SEXP unpack_read_environment(unpack_data *obj, sexp_info *info);
SEXP unpack_read_extptr(unpack_data *obj, sexp_info *info);
SEXP unpack_read_weakref(unpack_data *obj, sexp_info *info);
SEXP unpack_read_builtin(unpack_data *obj, sexp_info *info);
SEXP unpack_read_bcode(unpack_data *obj, sexp_info *info);

SEXP unpack_read_persistent_string(unpack_data *obj, sexp_info *info);

SEXP unpack_read_item(unpack_data *obj);

// Stream bits:
void stream_advance(stream_t *stream, R_xlen_t len);
void stream_move_to(stream_t *stream, R_xlen_t len);
void * stream_at(stream_t *stream, R_xlen_t len);
void stream_check_empty(stream_t *stream);

// The interface:
SEXP r_unpack_all(SEXP r_x);

SEXP r_sexptypes();
SEXP r_to_sexptype(SEXP x);
const char* to_sexptype(int type, const char * unknown);

// The internals
unpack_data * unpack_data_prepare(SEXP x);
void unpack_prepare(SEXP x, unpack_data *obj);
void unpack_add_attributes(SEXP s, sexp_info *info, unpack_data *obj);
void unpack_check_format(unpack_data *obj);
void unpack_check_version(unpack_data *obj);

SEXP unpack_inspect_item(unpack_data *obj);
void unpack_flags(int flags, sexp_info * info);
void unpack_sexp_info(unpack_data *obj, sexp_info *info);

SEXP init_read_ref(rds_index *index);
SEXP get_read_ref(unpack_data * obj, sexp_info *info, int index);
void add_read_ref(unpack_data * obj, SEXP value, sexp_info *info);

size_t unpack_write_string(unpack_data *obj, const char *s, size_t s_len,
                           const char **value);

#endif
