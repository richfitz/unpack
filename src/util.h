#include <R.h>
#include <Rinternals.h>
#include <stdbool.h>
bool scalar_logical(SEXP x, const char * name);
size_t scalar_size(SEXP x, const char *name);
const char * scalar_character(SEXP x, const char *name);

void assert_scalar_character(SEXP x, const char *name);

bool same_string(const char *a, const char *b, size_t len_a, size_t len_b);

void * check_extptr_valid(SEXP r_ptr, const char * name, bool closed_error);

size_t min2(size_t a, size_t b);
size_t max2(size_t a, size_t b);
