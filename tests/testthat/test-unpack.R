context("unpack")

test_that("scaffolding", {
  expect_null(unpack_all(raw()))
  expect_error(unpack_all(""), "Expected a raw string")
})
