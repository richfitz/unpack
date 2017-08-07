context("index")

## This case is the easiest
test_that("null", {
  xb <- serialize_binary(NULL)
  expect_equal(length(xb), 18L)

  idx <- unpack_index(xb)
  expect_equal(nrow(idx), 1)
  idx <- idx[1, ]

  expect_identical(idx[["id"]], 0L)
  expect_identical(idx[["parent"]], 0L)
  expect_identical(idx[["type"]], sexptypes[["NILVALUE"]])
  expect_identical(idx[["length"]], 0L)
  ## There's nothing here:
  expect_identical(idx[["start_object"]], 14L)
  expect_identical(idx[["start_data"]], 18L)
  expect_identical(idx[["start_attr"]], 18L)
  expect_identical(idx[["end"]], 18L)
})

## Next easiest is atomic number-like vectors.  To test this in a
## truely platform specific way we might need to get this passing back
## the results of sizeof()
test_that("logical", {
  x <- c(NA, TRUE, FALSE)
  xb <- serialize_binary(x)

  idx <- unpack_index(xb)
  expect_equal(nrow(idx), 1)
  idx <- idx[1, ]

  expect_equal(idx[["id"]], 0L)
  expect_equal(idx[["parent"]], 0L)
  expect_identical(idx[["type"]], sexptypes[["LGLSXP"]])
  expect_identical(idx[["length"]], 3L)

  expect_identical(idx[["start_object"]], 14L)
  expect_identical(idx[["start_data"]], 14L + 2L * 4L)
  expect_identical(idx[["start_attr"]], 14L + 2L * 4L + 3L * 4L)
  expect_identical(idx[["end"]], 14L + 2L * 4L + 3L * 4L)
})

test_that("integer", {
  x <- c(NA_integer_, 1L, -1000L)
  xb <- serialize_binary(x)

  idx <- unpack_index(xb)
  expect_equal(nrow(idx), 1)
  idx <- idx[1, ]

  expect_equal(idx[["id"]], 0L)
  expect_equal(idx[["parent"]], 0L)
  expect_identical(idx[["type"]], sexptypes[["INTSXP"]])
  expect_identical(idx[["length"]], 3L)

  expect_identical(idx[["start_object"]], 14L)
  expect_identical(idx[["start_data"]], 14L + 2L * 4L)
  expect_identical(idx[["start_attr"]], 14L + 2L * 4L + 3L * 4L)
  expect_identical(idx[["end"]], 14L + 2L * 4L + 3L * 4L)
})

## Then let's try a character vector; this is more challenging
test_that("character", {
  x <- c("hello", NA_character_, "", "world!")
  xb <- serialize_binary(x)

  idx <- unpack_index(xb)

  expect_equal(nrow(idx), 5L) # 1 for STRSXP, 4 x CHARSXP
  expect_equal(idx[, "id"], 0:4)
  expect_equal(idx[, "parent"], rep_len(0L, 5L))
  expect_equal(idx[, "type"],
               unname(sexptypes[rep(c("STRSXP", "CHARSXP"), c(1, 4))]))
  expect_equal(idx[, "length"],
               c(4L, 5L, -1L, 0L, 6L))

  ## The two words
  expect_equal(
    rawToChar(xb[(idx[2L, "start_data"] + 1):(idx[2L, "start_attr"])]),
    "hello")
  expect_equal(
    rawToChar(xb[(idx[5L, "start_data"] + 1):(idx[5L, "start_attr"])]),
    "world!")
  ## These have zero data
  expect_true(idx[3, "start_attr"] == idx[3, "end"])
  expect_true(idx[4, "start_attr"] == idx[4, "end"])
})
