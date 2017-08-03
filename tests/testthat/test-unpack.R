context("unpack")

test_that("scaffolding", {
  expect_null(unpack_all(raw()))
  expect_error(unpack_all(""), "Expected a raw string")
})

test_that("simplest case", {
  bytes1 <- serialize(NULL, NULL, xdr = FALSE)
  bytes2 <- serialize(c(1L, 9L), NULL, xdr = FALSE)
  bytes3 <- serialize(runif(4), NULL, xdr = FALSE)
  unpack_inspect(bytes1)
  unpack_inspect(bytes2)
  unpack_inspect(bytes3)

  unpack_all(bytes1)
  unpack_all(bytes2)
  unpack_all(bytes3)
})
