#include "buffer.h"

buffer_t * buffer_create(unsigned char *data, R_xlen_t len) {
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

void * buffer_at(buffer_t *buffer, R_xlen_t pos) {
  if (pos > buffer->len) {
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
  case ASCII:
  case XDR:
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
