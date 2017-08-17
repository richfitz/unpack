#ifndef UNPACK_UNPACK_H
#define UNPACK_UNPACK_H

#include <R.h>
#include <Rinternals.h>
#include <stdbool.h>

#include "structures.h"
#include "buffer.h"

SEXP r_unpack_all(SEXP r_x);
SEXP unpack_read_item(unpack_data *obj);

SEXP unpack_read_vector_integer(unpack_data *obj, sexp_info *info);
SEXP unpack_read_vector_real(unpack_data *obj, sexp_info *info);
SEXP unpack_read_vector_complex(unpack_data *obj, sexp_info *info);
SEXP unpack_read_vector_raw(unpack_data *obj, sexp_info *info);
SEXP unpack_read_vector_character(unpack_data *obj, sexp_info *info);
SEXP unpack_read_vector_generic(unpack_data *obj, sexp_info *info);

SEXP unpack_read_pairlist(unpack_data *obj, sexp_info *info);

SEXP unpack_read_charsxp(unpack_data *obj, sexp_info *info);
SEXP unpack_read_symbol(unpack_data *obj, sexp_info *info);

int unpack_read_ref_index(unpack_data *obj, sexp_info *info);
SEXP unpack_read_ref(unpack_data *obj, sexp_info *info);

SEXP unpack_read_package(unpack_data *obj, sexp_info *info);
SEXP unpack_read_namespace(unpack_data *obj, sexp_info *info);
SEXP unpack_read_environment(unpack_data *obj, sexp_info *info);

SEXP unpack_read_extptr(unpack_data *obj, sexp_info *info);
SEXP unpack_read_weakref(unpack_data *obj, sexp_info *info);

SEXP unpack_read_persist(unpack_data *obj, sexp_info *info);
SEXP unpack_read_persistent_string(unpack_data *obj, sexp_info *info);

SEXP unpack_read_builtin(unpack_data *obj, sexp_info *info);
SEXP unpack_read_bcode(unpack_data *obj, sexp_info *info);

// The internals
void unpack_add_attributes(SEXP s, sexp_info *info, unpack_data *obj);
void unpack_flags(int flags, sexp_info * info);

unpack_data * unpack_data_create(SEXP r_x);
void unpack_prepare(SEXP r_x, unpack_data *obj);
void unpack_check_format(unpack_data *obj);
void unpack_check_version(unpack_data *obj);

SEXP init_read_ref(rds_index *index);
SEXP get_read_ref(unpack_data * obj, sexp_info *info, int index);
void add_read_ref(unpack_data * obj, SEXP value, sexp_info *info);

size_t unpack_write_string(unpack_data *obj, const char *s, size_t s_len,
                           const char **value);

#endif
