context("search")

test_that("search_attribute", {
  x <- 1:5
  attr(x, "class") <- "mything"
  names(x) <- c("one", "two", "three", "four", "five")
  xb <- serialize_binary(x)
  idx_ptr <- unpack_index(xb, TRUE)
  idx <- unpack_index_as_matrix(idx_ptr)

  ## Hell yes:
  i_c <- index_search_attribute(xb, idx_ptr, 0L, "class")
  i_n <- index_search_attribute(xb, idx_ptr, 0L, "names")
  i_x <- index_search_attribute(xb, idx_ptr, 0L, "x")
  expect_equal(i_c, 4L)
  expect_equal(i_n, 9L)
  expect_equal(i_x, NA_integer_)

  expect_identical(unpack_extract(xb, idx_ptr, i_c), class(x))
  expect_identical(unpack_extract(xb, idx_ptr, i_n), names(x))
})

test_that("attribute redirect", {
  inner <- structure(setNames(1:5, c("one", "two", "three", "four", "five")),
                     class = "inner")
  outer <- structure(list(first = inner, second = NULL),
                     class = "outer")
  xb <- serialize_binary(outer)

  idx_ptr <- unpack_index(xb, TRUE)
  idx <- unpack_index_as_matrix(idx_ptr)

  ## The outer case involves some reference lookups:
  i1_c <- index_search_attribute(xb, idx_ptr, 0L, "class")
  i1_n <- index_search_attribute(xb, idx_ptr, 0L, "names")
  i1_x <- index_search_attribute(xb, idx_ptr, 0L, "x")
  expect_identical(unpack_extract(xb, idx_ptr, i1_c), class(outer))
  expect_identical(unpack_extract(xb, idx_ptr, i1_n), names(outer))
  expect_identical(i1_x, NA_integer_)

  i <- index_find_nth_child(idx_ptr, 0L, 1L)
  expect_identical(to_sexptype(idx[i + 1L, "type"]), "INTSXP")

  i2_c <- index_search_attribute(xb, idx_ptr, i, "class")
  i2_n <- index_search_attribute(xb, idx_ptr, i, "names")
  i2_x <- index_search_attribute(xb, idx_ptr, i, "x")
  expect_identical(unpack_extract(xb, idx_ptr, i2_c), class(inner))
  expect_identical(unpack_extract(xb, idx_ptr, i2_n), names(inner))
  expect_identical(i2_x, NA_integer_)
})

test_that("search_character", {
  x <- 1:5
  attr(x, "class") <- "mything"
  names(x) <- c("one", "two", "three", "four", "five")
  xb <- serialize_binary(x)
  idx_ptr <- unpack_index(xb, TRUE)
  idx <- unpack_index_as_matrix(idx_ptr)
  i <- index_search_attribute(xb, idx_ptr, 0L, "names")

  for (j in seq_along(x)) {
    expect_identical(index_search_character(xb, idx_ptr, i, names(x)[[j]]), j)
  }
  expect_identical(index_search_character(xb, idx_ptr, i, "seven"),
                   NA_integer_)
})
