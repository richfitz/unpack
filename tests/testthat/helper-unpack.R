serialize_binary <- function(x) {
  serialize(x, NULL, xdr = FALSE)
}

expect_roundtrip <- function(x) {
  testthat::expect_identical(unpack_all(serialize_binary(x)), x)
}
