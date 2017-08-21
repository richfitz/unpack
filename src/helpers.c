#include "helpers.h"
#include "upstream.h"

SEXP r_sexptypes() {
  size_t n = 35;
  SEXP nms = PROTECT(allocVector(STRSXP, n));
  SEXP ret = PROTECT(allocVector(INTSXP, n));
  setAttrib(ret, R_NamesSymbol, nms);
  int *v = INTEGER(ret);

  size_t i = 0;

  v[i] = SYMSXP; // 1
  SET_STRING_ELT(nms, i++, mkChar("SYMSXP"));
  v[i] = LISTSXP; // 2
  SET_STRING_ELT(nms, i++, mkChar("LISTSXP"));
  v[i] = CLOSXP; // 3
  SET_STRING_ELT(nms, i++, mkChar("CLOSXP"));
  v[i] = ENVSXP; // 4
  SET_STRING_ELT(nms, i++, mkChar("ENVSXP"));
  v[i] = PROMSXP; // 5
  SET_STRING_ELT(nms, i++, mkChar("PROMSXP"));
  v[i] = LANGSXP; // 6
  SET_STRING_ELT(nms, i++, mkChar("LANGSXP"));
  v[i] = SPECIALSXP; // 7
  SET_STRING_ELT(nms, i++, mkChar("SPECIALSXP"));
  v[i] = BUILTINSXP; // 8
  SET_STRING_ELT(nms, i++, mkChar("BUILTINSXP"));
  v[i] = CHARSXP; // 9
  SET_STRING_ELT(nms, i++, mkChar("CHARSXP"));
  v[i] = LGLSXP; // 10
  SET_STRING_ELT(nms, i++, mkChar("LGLSXP"));
  v[i] = INTSXP; // 13
  SET_STRING_ELT(nms, i++, mkChar("INTSXP"));
  v[i] = REALSXP; // 14
  SET_STRING_ELT(nms, i++, mkChar("REALSXP"));
  v[i] = CPLXSXP; // 15
  SET_STRING_ELT(nms, i++, mkChar("CPLXSXP"));
  v[i] = STRSXP; // 16
  SET_STRING_ELT(nms, i++, mkChar("STRSXP"));
  v[i] = DOTSXP; // 17
  SET_STRING_ELT(nms, i++, mkChar("DOTSXP"));
  v[i] = VECSXP; // 19
  SET_STRING_ELT(nms, i++, mkChar("VECSXP"));
  v[i] = EXPRSXP; // 20
  SET_STRING_ELT(nms, i++, mkChar("EXPRSXP"));
  v[i] = BCODESXP; // 21
  SET_STRING_ELT(nms, i++, mkChar("BCODESXP"));
  v[i] = EXTPTRSXP; // 22
  SET_STRING_ELT(nms, i++, mkChar("EXTPTRSXP"));
  v[i] = WEAKREFSXP; // 23
  SET_STRING_ELT(nms, i++, mkChar("WEAKREFSXP"));
  v[i] = RAWSXP; // 24
  SET_STRING_ELT(nms, i++, mkChar("RAWSXP"));
  v[i] = S4SXP; // 25
  SET_STRING_ELT(nms, i++, mkChar("S4SXP"));
  v[i] = BASEENV_SXP; // 241
  SET_STRING_ELT(nms, i++, mkChar("BASEENV"));
  v[i] = EMPTYENV_SXP; // 242
  SET_STRING_ELT(nms, i++, mkChar("EMPTYENV"));
  v[i] = GENERICREFSXP; // 245
  SET_STRING_ELT(nms, i++, mkChar("GENERICREFSXP"));
  v[i] = CLASSREFSXP; // 246
  SET_STRING_ELT(nms, i++, mkChar("CLASSREFSXP"));
  v[i] = PERSISTSXP; // 247
  SET_STRING_ELT(nms, i++, mkChar("PERSISTSXP"));
  v[i] = PACKAGESXP; // 248
  SET_STRING_ELT(nms, i++, mkChar("PACKAGESXP"));
  v[i] = NAMESPACESXP; // 249
  SET_STRING_ELT(nms, i++, mkChar("NAMESPACESXP"));
  v[i] = BASENAMESPACE_SXP; // 250
  SET_STRING_ELT(nms, i++, mkChar("BASENAMESPACE"));
  v[i] = MISSINGARG_SXP; // 251
  SET_STRING_ELT(nms, i++, mkChar("MISSINGARG"));
  v[i] = UNBOUNDVALUE_SXP; // 252
  SET_STRING_ELT(nms, i++, mkChar("UNBOUNDVALUE"));
  v[i] = GLOBALENV_SXP; // 253
  SET_STRING_ELT(nms, i++, mkChar("GLOBALENV"));
  v[i] = NILVALUE_SXP; // 254
  SET_STRING_ELT(nms, i++, mkChar("NILVALUE"));
  v[i] = REFSXP; // 255
  SET_STRING_ELT(nms, i++, mkChar("REFSXP"));

  UNPROTECT(2);
  return ret;
}

SEXP r_to_sexptype(SEXP x) {
  int * v = INTEGER(x);
  SEXP ret = PROTECT(allocVector(STRSXP, length(x)));
  for (R_xlen_t i = 0; i < length(x); ++i) {
    const char *name = to_sexptype(v[i], NULL);
    SET_STRING_ELT(ret, i, name == NULL ? NA_STRING : mkChar(name));
  }
  UNPROTECT(1);
  return ret;
}

SEXP r_to_typeof(SEXP x) {
  int * v = INTEGER(x);
  SEXP ret = PROTECT(allocVector(STRSXP, length(x)));
  for (R_xlen_t i = 0; i < length(x); ++i) {
    const char *name = to_typeof(v[i], NULL);
    SET_STRING_ELT(ret, i, name == NULL ? NA_STRING : mkChar(name));
  }
  UNPROTECT(1);
  return ret;
}

const char* to_sexptype(int type, const char * unknown) {
  switch(type) {
  case SYMSXP:            return "SYMSXP";
  case LISTSXP:           return "LISTSXP";
  case CLOSXP:            return "CLOSXP";
  case ENVSXP:            return "ENVSXP";
  case PROMSXP:           return "PROMSXP";
  case LANGSXP:           return "LANGSXP";
  case SPECIALSXP:        return "SPECIALSXP";
  case BUILTINSXP:        return "BUILTINSXP";
  case CHARSXP:           return "CHARSXP";
  case LGLSXP:            return "LGLSXP";
  case INTSXP:            return "INTSXP";
  case REALSXP:           return "REALSXP";
  case CPLXSXP:           return "CPLXSXP";
  case STRSXP:            return "STRSXP";
  case DOTSXP:            return "DOTSXP";
  case VECSXP:            return "VECSXP";
  case EXPRSXP:           return "EXPRSXP";
  case BCODESXP:          return "BCODESXP";
  case EXTPTRSXP:         return "EXTPTRSXP";
  case WEAKREFSXP:        return "WEAKREFSXP";
  case RAWSXP:            return "RAWSXP";
  case S4SXP:             return "S4SXP";
    // These  are not real types
  case BASEENV_SXP:       return "BASEENV";
  case EMPTYENV_SXP:      return "EMPTYENV";
  case GENERICREFSXP:     return "GENERICREFSXP";
  case CLASSREFSXP:       return "CLASSREFSXP";
  case PERSISTSXP:        return "PERSISTSXP";
  case PACKAGESXP:        return "PACKAGESXP";
  case NAMESPACESXP:      return "NAMESPACESXP";
  case BASENAMESPACE_SXP: return "BASENAMESPACE";
  case MISSINGARG_SXP:    return "MISSINGARG";
  case UNBOUNDVALUE_SXP:  return "UNBOUNDVALUE";
  case GLOBALENV_SXP:     return "GLOBALENV";
  case NILVALUE_SXP:      return "NILVALUE";
  case REFSXP:            return "REFSXP";
    // Unlikely
  default:                return unknown;
  }
}

const char * to_typeof(int type, const char * unknown) {
  switch(type) {
  case SYMSXP:            return "symbol";
  case LISTSXP:           return "pairlist";
  case CLOSXP:            return "function";
  case ENVSXP:            return "environment";
  case PROMSXP:           return "promise";
  case LANGSXP:           return "language"; // I *think*
  case SPECIALSXP:        return "SPECIALSXP"; // not sure
  case BUILTINSXP:        return "builtin";
  case CHARSXP:           return "CHARSXP"; // internal type
  case LGLSXP:            return "logical";
  case INTSXP:            return "integer";
  case REALSXP:           return "double";
  case CPLXSXP:           return "complex";
  case STRSXP:            return "character";
  case DOTSXP:            return "DOTSXP"; // internal type
  case VECSXP:            return "list";
  case EXPRSXP:           return "expression";
  case BCODESXP:          return "BCODESXP"; // possibly internal?
  case EXTPTRSXP:         return "externalptr";
  case WEAKREFSXP:        return "weakref";
  case RAWSXP:            return "raw";
  case S4SXP:             return "S4";
    // These  are not real types
  case BASEENV_SXP:       return "environment";
  case EMPTYENV_SXP:      return "environment";
  case GENERICREFSXP:     return "GENERICREFSXP"; // internal type
  case CLASSREFSXP:       return "CLASSREFSXP"; // internal type
  case PERSISTSXP:        return "PERSISTSXP"; // internal type
  case PACKAGESXP:        return "environment";
  case NAMESPACESXP:      return "environment";
  case BASENAMESPACE_SXP: return "environment";
  case MISSINGARG_SXP:    return "MISSINGARG"; // internal type
  case UNBOUNDVALUE_SXP:  return "UNBOUNDVALUE"; // internal type
  case GLOBALENV_SXP:     return "environment";
  case NILVALUE_SXP:      return "NULL";
  case REFSXP:            return "REFSXP"; // internal type
    // Unlikely
  default:                return unknown;
  }
}
