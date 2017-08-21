context("xdr")

test_that("int", {
  n <- 12525L
  x <- tail(serialize(n, NULL), 4)
  expect_identical(xdr_read_int(x), n)
})

test_that("double", {
  n <- pi
  x <- tail(serialize(n, NULL), 8)
  expect_identical(xdr_read_double(x), n)
})

test_that("complex", {
  n <- pi + exp(1) * 1i
  x <- tail(serialize(n, NULL), 16)
  expect_identical(xdr_read_complex(x), n)
})
