context("find")

test_that("find_element: simple", {
  x <- list(a = 1, b = 2, c = 3)
  b <- serialize_binary(x)
  idx_ptr <- unpack_index(b, TRUE)
  idx <- unpack_index_as_matrix(idx_ptr)

  expect_error(index_find_element(idx_ptr, 0L, 0L),
               "Expected a nonzero positive size for 'i'")
  expect_error(index_find_element(idx_ptr, 0L, -1L),
               "Expected a positive size for 'i'")
  for (i in seq_along(x)) {
    expect_identical(index_find_nth_child(idx_ptr, 0L, i), i)
  }
  expect_identical(index_find_element(idx_ptr, 0L, 5L), NA_integer_)

  expect_error(index_find_element(idx_ptr, 1L, 1L),
               "Cannot index into a REALSXP")
})

test_that("find_attributes", {
  x <- list(a = 1, b = 2, c = 3)
  b <- serialize_binary(x)
  idx_ptr <- unpack_index(b, TRUE)
  idx <- unpack_index_as_matrix(idx_ptr)

  i <- index_find_attributes(idx_ptr, 0L)
  expect_identical(i, 4L)
  expect_equal(to_sexptype(idx[i + 1, "type"]), "LISTSXP")

  expect_identical(index_find_attributes(idx_ptr, 1L), NA_integer_)
})
