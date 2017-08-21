serialize_binary <- function(x, xdr = FALSE) {
  serialize(x, NULL, xdr = xdr)
}

expect_roundtrip <- function(x, xdr = FALSE) {
  testthat::expect_identical(unpack_all(serialize_binary(x, xdr)), x)
}

unpack_index <- function(x) {
  rdsi_get_index_matrix(rdsi_build(x))
}
