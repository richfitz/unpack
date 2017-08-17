#include "structures.h"

buffer_t * buffer_create(unsigned char *data, R_xlen_t len);
void buffer_advance(buffer_t *stream, R_xlen_t len);
void buffer_move_to(buffer_t *stream, R_xlen_t len);
void * buffer_at(buffer_t *stream, R_xlen_t len);
void buffer_check_empty(buffer_t *stream);
void buffer_read_bytes(buffer_t *buffer, R_xlen_t len, void *dest);
int buffer_read_char(buffer_t *buffer);
int buffer_read_integer(buffer_t *buffer);
void buffer_read_string(buffer_t *buffer, size_t len, char *dest);