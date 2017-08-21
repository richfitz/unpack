context("xdr")

test_that("int", {
  n <- 12525L
  x <- tail(serialize(n, NULL), 4)
  expect_identical(xdr_read_int(x), n)

  n <- sample.int(100000, 5)
  x <- tail(serialize(n, NULL), 4 * length(n))
  expect_identical(xdr_read_int(x), n)
})

test_that("double", {
  n <- pi
  x <- tail(serialize(n, NULL), 8)
  expect_identical(xdr_read_double(x), n)

  n <- runif(5)
  x <- tail(serialize(n, NULL), 8 * length(n))
  expect_identical(xdr_read_double(x), n)
})

test_that("complex", {
  n <- pi + exp(1) * 1i
  x <- tail(serialize(n, NULL), 16)
  expect_identical(xdr_read_complex(x), n)

  n <- runif(5) + runif(5) * 1i
  x <- tail(serialize(n, NULL), 16 * length(n))
  expect_identical(xdr_read_complex(x), n)
})
