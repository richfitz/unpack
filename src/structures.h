#ifndef UNPACK_STRUCTURES_H
#define UNPACK_STRUCTURES_H

#include <R.h>
#include <Rinternals.h>
#include <stdbool.h>

// The buffer
typedef enum {
  ASCII,
  BINARY,
  XDR
} serialisation_format;

// NOTE: Using R's long size types here in order to support
// unserializing large objects.  We don't support resizing the buffer
// because we're just going to try and read from this rather than
// allocate it.  See R-ints for what the right thing to do here is.
typedef struct {
  R_xlen_t len; // total capacity
  R_xlen_t pos; // current position
  unsigned char *data;
  serialisation_format format;
  int depth;
} buffer_t;


#endif
