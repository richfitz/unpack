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
  //  int length;
  bool is_object;
  bool has_attr;
  bool has_tag;
} sexp_info;

// NOTE: Using R's long size types here in order to support
// unserializing large objects.  We don't support resizing the buffer
// because we're just going to try and read from this rather than
// allocate it.  See R-ints for what the right thing to do here is.
typedef struct stream_st {
  size_t size;
  size_t count;
  unsigned char *buf;
  serialisation_format format;
} *stream_t;

void stream_read_bytes(stream_t stream, void *buf, size_t len);
int stream_read_char(stream_t stream);
int stream_read_integer(stream_t stream);
void stream_advance(stream_t stream, size_t len);

// The interface:
SEXP r_unpack_all(SEXP x);

// The internals
SEXP unpack_unserialise(stream_t stream);
SEXP unpack_read_item(stream_t stream);
void unpack_check_format(stream_t stream);
void unpack_check_version(stream_t stream);

SEXP unpack_inspect_item(stream_t stream);
void unpack_flags(int flags, sexp_info * info);
