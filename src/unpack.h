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
  // extraction and I might remove them, but am not sure.
  R_xlen_t length;
  R_xlen_t start_object;
  R_xlen_t start_data;
  R_xlen_t start_attr;
  R_xlen_t end;
  R_xlen_t parent;
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
typedef struct stream_st {
  R_xlen_t size;
  R_xlen_t count;
  unsigned char *buf;
  serialisation_format format;
  int depth;
} *stream_t;

int stream_read_char(stream_t stream);
int stream_read_integer(stream_t stream);
void stream_read_string(stream_t stream, char *buf, int length);

R_xlen_t stream_read_length(stream_t stream);

void stream_read_bytes(stream_t stream, void *buf, R_xlen_t len);

SEXP stream_read_vector_integer(stream_t stream, sexp_info *info);
SEXP stream_read_vector_real(stream_t stream, sexp_info *info);
SEXP stream_read_vector_complex(stream_t stream, sexp_info *info);
SEXP stream_read_vector_raw(stream_t stream, sexp_info *info);
SEXP stream_read_vector_character(stream_t stream, sexp_info *info);
SEXP stream_read_vector_generic(stream_t stream, sexp_info *info);
SEXP stream_read_charsxp(stream_t stream, sexp_info *info);

void stream_advance(stream_t stream, R_xlen_t len);

// The interface:
SEXP r_unpack_all(SEXP x);
SEXP r_unpack_inspect(SEXP x);
SEXP r_sexptypes();
SEXP r_to_sexptype(SEXP x);

// The internals
void unpack_prepare(SEXP x, stream_t stream);
SEXP unpack_unserialise(stream_t stream);
SEXP unpack_read_item(stream_t stream);
void unpack_add_attributes(SEXP s, sexp_info *info, stream_t stream);
void unpack_check_format(stream_t stream);
void unpack_check_version(stream_t stream);

SEXP unpack_inspect_item(stream_t stream);
void unpack_flags(int flags, sexp_info * info);
void unpack_sexp_info(stream_t stream, sexp_info *info);

#endif
