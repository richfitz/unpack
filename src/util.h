#include <R.h>
#include <Rinternals.h>
#include <stdbool.h>
bool scalar_logical(SEXP x, const char * name);
size_t scalar_size(SEXP x, const char *name);
const char * scalar_character(SEXP x, const char *name);

void assert_scalar_character(SEXP x, const char *name);

bool same_string(const char *a, const char *b, size_t len_a, size_t len_b);
