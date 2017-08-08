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

test_that("throw on spare bytes", {
  xb <- c(serialize_binary(NULL), as.raw(c(0, 0, 0, 0)))
  expect_error(unpack_index(xb),
               "Did not consume all of raw vector: 4 bytes left")
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

test_that("pairlist", {
  x <- as.pairlist(list(1L, pi, TRUE))
  xb <- serialize_binary(x)
  idx <- unpack_index(xb)

  ## For a 3 element pair list there are *7* SEXPS
  expect <- c("LISTSXP", "INTSXP",
              "LISTSXP", "REALSXP",
              "LISTSXP", "LGLSXP",
              "NILVALUE")
  expect_equal(idx[, "type"], unname(sexptypes[expect]))
  expect_equal(idx[, "length"], rep(1:0, c(6, 1)))
  expect_equal(idx[, "parent"], c(0, 0, 0, 2, 2, 4, 4))
})

## Where it gets interesting is attributes, because these are the
## things we eventually have to be able to access
test_that("attributes", {
  x <- setNames(1:5, c("one", "two", "three", "four", "five"))
  xb <- serialize_binary(x)
  idx <- unpack_index(xb)

  ## So, what is going on here?
  expect_equal(unname(idx[1, "type"]), sexptypes[["INTSXP"]])
  expect_equal(unname(idx[1, "length"]), 5L)

  ## Followed by a dotted list, which is the attributes
  expect_equal(unname(idx[2, "type"]), sexptypes[["LISTSXP"]])
  expect_equal(unname(idx[2, "start_object"]),
               unname(idx[1, "start_attr"]))

  ## We have three children here.  For dotted lists, attributes come
  ## first (and tag), then cdr then car
  idx[idx[, "id"] == 1L, ]
  i <- which(idx[, "parent"] == 1L)

  to_sexptype(idx[i, "type"])

  ## This is the symbol
  expect_equal(unname(idx[i[1], "type"]),
               unname(sexptypes[["SYMSXP"]]))
  ## And this is the actual names element
  j <- which(idx[, "parent"] == idx[i[1], "id"])
  expect_equal(unname(idx[j, "type"]),
               unname(sexptypes[["CHARSXP"]]))
  expect_equal(xb[(idx[j, "start_data"] + 1L):(idx[j, "start_attr"])],
               charToRaw("names"))
})

test_that("multiple attributes", {
  x <- 1:5
  attr(x, "class") <- "mything"
  names(x) <- c("one", "two", "three", "four", "five")
  xb <- serialize_binary(x)
  idx_ptr <- unpack_index(xb, TRUE)
  idx <- unpack_index_as_matrix(idx_ptr)

  ## Hell yes:
  i_c <- index_find_attribute(idx_ptr, 0L, "class", xb)
  i_n <- index_find_attribute(idx_ptr, 0L, "names", xb)
  i_x <- index_find_attribute(idx_ptr, 0L, "x", xb)
  expect_equal(i_c, 4L)
  expect_equal(i_n, 9L)
  expect_equal(i_x, NA_integer_)

  expect_identical(unpack_extract(xb, idx_ptr, i_c), class(x))
  expect_identical(unpack_extract(xb, idx_ptr, i_n), names(x))
})

test_that("attributed attributes", {
  x <- structure(pi, a = 1, bar = 2)
  v <- structure("hello", x = x, y = TRUE)
  idx <- unpack_index(serialize_binary(v))
  idx[6, ]
})

test_that("access list elements", {
  x <- list(1, "two", 1:3)
  xb <- serialize_binary(x)
  idx <- unpack_index(xb, TRUE)
  for (i in 1:3) {
    expect_identical(unpack_extract_element(xb, idx, 0L, i), x[[i]])
  }
  expect_null(unpack_extract_element(xb, idx, 0L, 4L, FALSE))
  expect_error(unpack_extract_element(xb, idx, 0L, 4L, TRUE),
               "Index 4 out of bounds; must be on [1, 3]", fixed = TRUE)

  expect_error(unpack_extract_element(xb, idx, 0L, -4L, TRUE),
               "Expected a positive size for 'i'")
  expect_error(unpack_extract_element(xb, idx, 0L, 0L, TRUE),
               "Expected a nonzero positive size for 'i'")
})
