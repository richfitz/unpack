#include "buffer.h"
#include "xdr.h"

buffer_t * buffer_create(const data_t *data, R_xlen_t len) {
  buffer_t *buffer = (buffer_t *)R_alloc(1, sizeof(buffer_t));
  buffer->pos = 0;
  buffer->len = len;
  buffer->data = data;
  return buffer;
}

void buffer_advance(buffer_t *buffer, R_xlen_t len) {
  if (buffer->pos + len > buffer->len) {
    Rf_error("buffer overflow");
  }
  buffer->pos += len;
}

void buffer_move_to(buffer_t *buffer, R_xlen_t pos) {
  if (pos > buffer->len) {
    Rf_error("buffer overflow");
  }
  buffer->pos = pos;
}

const data_t * buffer_at(buffer_t *buffer, R_xlen_t pos, size_t len) {
  if (pos + (R_xlen_t)len > buffer->len) {
    Rf_error("buffer overflow");
  }
  return buffer->data + pos;
}

void buffer_check_empty(buffer_t *buffer) {
  if (buffer->pos != buffer->len) {
    Rf_error("Did not consume all of raw vector: %d bytes left",
             buffer->len - buffer->pos);
  }
}

// InBytes
void buffer_read_bytes(buffer_t *buffer, R_xlen_t len, void *dest) {
  if (buffer->pos + len > buffer->len) {
    Rf_error("buffer overflow (trying to move to %d of %d)",
             buffer->pos + len, buffer->len);
  }
  memcpy(dest, buffer->data + buffer->pos, len);
  buffer->pos += len;
}

// InChar
int buffer_read_char(buffer_t *buffer) {
  if (buffer->pos >= buffer->len) {
    Rf_error("buffer overflow");
  }
  return buffer->data[buffer->pos++];
}

// InInteger
int buffer_read_integer(buffer_t *buffer) {
  // char word[128];
  // char buf[128];
  int i;
  switch (buffer->format) {
  case BINARY:
    buffer_read_bytes(buffer, sizeof(int), &i);
    return i;
  case XDR:
    xdr_read_int(buffer->data + buffer->pos, &i);
    buffer_advance(buffer, sizeof(int));
    return i;
  case ASCII:
  default:
    Rf_error("not implemented (read_integer)");
  }
  return NA_INTEGER;
}

// InString
void buffer_read_string(buffer_t *buffer, size_t len, char *dest) {
  switch (buffer->format) {
  case BINARY:
  case XDR:
    buffer_read_bytes(buffer, len, dest);
    break;
  case ASCII:
  default:
    Rf_error("not implemented (read_string)");
  }
}

// ReadLENGTH
R_xlen_t buffer_read_length(buffer_t *buffer) {
  int len = buffer_read_integer(buffer);
#ifdef LONG_VECTOR_SUPPORT
  if (len < -1)
    Rf_error("negative serialized length for vector");
  if (len == -1) {
    unsigned int len1, len2;
    len1 = buffer_read_integer(buffer); /* upper part */
    len2 = buffer_read_integer(buffer); /* lower part */
    R_xlen_t xlen = len1;
    /* sanity check for now */
    if (len1 > 65536)
      Rf_error("invalid upper part of serialized vector length");
    return (xlen << 32) + len2;
  } else return len;
#else
  if (len < 0)
    Rf_error("negative serialized vector length:\nperhaps long vector from 64-bit version of R?");
  return len;
#endif
}

void buffer_read_int_vector(buffer_t *buffer, size_t len, int *dest) {
  size_t nbytes = (size_t)(sizeof(int) * len);
  switch (buffer->format) {
  case BINARY:
    buffer_read_bytes(buffer, nbytes, dest);
    break;
  case XDR:
    xdr_read_int_vector(buffer->data + buffer->pos, len, dest);
    buffer_advance(buffer, nbytes);
    break;
  case ASCII:
  default:
    Rf_error("not implemented (read_int_vector)");
  }
}

void buffer_read_double_vector(buffer_t *buffer, size_t len, double *dest) {
  size_t nbytes = (size_t)(sizeof(double) * len);
  switch (buffer->format) {
  case BINARY:
    buffer_read_bytes(buffer, nbytes, dest);
    break;
  case XDR:
    xdr_read_double_vector(buffer->data + buffer->pos, len, dest);
    buffer_advance(buffer, nbytes);
    break;
  case ASCII:
  default:
    Rf_error("not implemented (read_double_vector)");
  }
}

void buffer_read_complex_vector(buffer_t *buffer, size_t len, Rcomplex *dest) {
  size_t nbytes = (size_t)(sizeof(Rcomplex) * len);
  switch (buffer->format) {
  case BINARY:
    buffer_read_bytes(buffer, nbytes, dest);
    break;
  case XDR:
    xdr_read_complex_vector(buffer->data + buffer->pos, len, dest);
    buffer_advance(buffer, nbytes);
    break;
  case ASCII:
  default:
    Rf_error("not implemented (read_complex_vector)");
  }
}
