#ifndef UNPACK_STRUCTURES_H
#define UNPACK_STRUCTURES_H

#include <R.h>
#include <Rinternals.h>
#include <stdbool.h>

typedef unsigned char data_t;

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
  const data_t *data;
  serialisation_format format;
} buffer_t;

typedef struct {
  int flags;
  SEXPTYPE type;
  int levels;
  R_xlen_t id;

  // NOTE: These are here for the index; they're wasted space for the
  // extraction and I might remove them, but am not sure.  They would
  // be better in a second structure that we keep around just for the
  // indexing, I think.  These will be uninitialised and *must not be
  // read* unless they have come from the index.
  R_xlen_t length;
  R_xlen_t start_object;
  R_xlen_t start_data;
  R_xlen_t start_attr;
  R_xlen_t end;
  R_xlen_t parent;
  R_xlen_t refid; // Either a from or a to

  // These come from `flags` via a bitmask (actually so do type and
  // levels).  They'll be stored as single bits so I am chucking them
  // at the end.
  bool is_object;
  bool has_attr;
  bool has_tag;
} sexp_info_t;

// This is our index; it is not a big object.
typedef struct {
  // A vector of useful indices
  const sexp_info_t *objects;
  // The length of 'objects'
  R_xlen_t len;
  // The number of references (this can be computed from objects/len
  // fairly cheaply, but we might as well store it)
  size_t n_refs;
  // The total length of the data
  R_xlen_t len_data;
} rds_index_t;

// And this is the one for building with
typedef struct {
  sexp_info_t *objects;
  R_xlen_t len;
  size_t n_refs;
  R_xlen_t len_data;
} rds_index_mutable_t;

// This will be the object that we pass back to R; it contains the
// data, index and a set of reference objects.  These will be moved
// into place into the unpack_data object as needed.  The ref_objects
// one might move out of here.
typedef struct {
  const data_t * data;
  R_xlen_t data_len;
  const rds_index_t * index;
  SEXP ref_objects;
} rdsi_t;

typedef struct {
  // The actual data to read
  buffer_t * buffer;
  // Storage for reference objects. Stored as a protected pairlist
  //
  // (data . NILVALUE)
  //
  // so that we can safely add objects into the data and resize as needed.
  SEXP ref_objects;
  // An index, if we are using one
  const rds_index_t * index;

  // The object position in the stream.  This is used to assign the
  // next object id into a sexp_info_t (which is then used to organise
  // the reference table) and used when reading so that we can skip
  // ahead over bits that we have already read.
  R_xlen_t count;
} unpack_data_t;

#endif
