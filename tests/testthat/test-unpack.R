context("unpack")

test_that("scaffolding", {
  expect_null(unpack_all(raw()))
  expect_error(unpack_all(""), "Expected a raw string")
})

test_that("logical", {
  expect_roundtrip(logical(0))
  expect_roundtrip(c(NA, TRUE, FALSE))
})

test_that("integer", {
  expect_roundtrip(integer(0))
  expect_roundtrip(c(NA_integer_, 1L, 999L))
})

test_that("numeric", {
  expect_roundtrip(numeric(0))
  expect_roundtrip(c(NA_real_, runif(5)))
})

test_that("complex", {
  expect_roundtrip(complex(0))
  expect_roundtrip(c(1+2i, NA, runif(2)))
})

test_that("raw", {
  expect_roundtrip(raw(0))
  expect_roundtrip(charToRaw("hello"))
})

test_that("character", {
  expect_roundtrip(character(0))
  expect_roundtrip(c("hello"))
  expect_roundtrip(c("hello", "world"))
  expect_roundtrip(c("", NA_character_))
  expect_roundtrip(NA_character_)
})
