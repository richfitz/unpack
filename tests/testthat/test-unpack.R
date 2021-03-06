context("unpack")

test_that("scaffolding", {
  ##  expect_null(unpack_all(raw()))
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
  expect_roundtrip(c(NA_real_, runif(5)), TRUE)
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

test_that("list", {
  expect_roundtrip(list())
  expect_roundtrip(list("a"))
  expect_roundtrip(list(list(list("a"))))
})

test_that("attributes", {
  expect_roundtrip(structure(1:5, names = letters[1:5]))
  expect_roundtrip(structure(1:5, names = letters[1:5], foo = "x"))
})

test_that("references", {
  expect_roundtrip(structure(list(structure(1, class = "foo")), class = "bar"))

  a <- 1:3
  expect_roundtrip(list(a, a))
})

test_that("namespace", {
  expect_roundtrip(asNamespace("stats"))
})

test_that("package", {
  p <- as.environment("package:stats")
  x <- suppressWarnings(serialize_binary(p))
  expect_identical(unpack_all(x), p)
})

test_that("env", {
  e <- new.env(parent = emptyenv())
  e$a <- 1L
  e2 <- unpack_all(serialize_binary(e))
  expect_identical(parent.env(e), parent.env(e2))
  expect_identical(ls(e), ls(e2))
  expect_identical(as.list(e), as.list(e2))
})

test_that("extptr", {
  b <- as.raw(c(0x42, 0x0a, 0x02, 0x00, 0x00, 0x00, 0x00, 0x04, 0x03,
                0x00, 0x00, 0x03, 0x02, 0x00, 0x16, 0x00, 0x00, 0x00,
                0xfe, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00))
  ptr <- unserialize(b) # Using R's functions
  expect_roundtrip(ptr)
})
