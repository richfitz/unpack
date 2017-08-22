context("find")

test_that("find_element: simple", {
  x <- list(a = 1, b = 2, c = 3)
  b <- serialize_binary(x)
  rdsi <- rdsi_build(b)
  idx <- rdsi_get_index_matrix(rdsi)

  expect_error(index_find_element(rdsi, 0L, 0L),
               "Expected a nonzero positive size for 'i'")
  expect_error(index_find_element(rdsi, 0L, -1L),
               "Expected a positive size for 'i'")
  for (i in seq_along(x)) {
    expect_identical(index_find_nth_child(rdsi, 0L, i), i)
  }
  expect_identical(index_find_element(rdsi, 0L, 5L), NA_integer_)

  expect_error(index_find_element(rdsi, 1L, 1L),
               "Cannot index into a REALSXP")
})

test_that("find_attributes", {
  x <- list(a = 1, b = 2, c = 3)
  b <- serialize_binary(x)
  rdsi <- rdsi_build(b)
  idx <- rdsi_get_index_matrix(rdsi)

  i <- index_find_attributes(rdsi, 0L)
  expect_identical(i, 4L)
  expect_equal(to_sexptype(idx[i + 1, "type"]), "LISTSXP")

  expect_identical(index_find_attributes(rdsi, 1L), NA_integer_)
})

## Just need to get some decent testing in here.
test_that("find id", {
  rdsi <- rdsi_build(serialize_binary(mtcars))
  idx <- rdsi_get_index_matrix(rdsi)

  at <- idx[, "start_object"][10]
  i <- index_find_id_bisect(rdsi, at, 0L)

  at <- idx[1, "start_attr"]
  i <- index_find_id_bisect(rdsi, at, 0L)

  at2 <- idx[idx[, "parent"] == i, "start_object"]
  j1 <- vapply(at2, index_find_id_linear, integer(1), rdsi = rdsi, start_id = i)
  j2 <- vapply(at2, index_find_id_bisect, integer(1), rdsi = rdsi, start_id = i)
  expect_identical(j1, j2)

  for (el in j1) {
    at3 <- idx[idx[, "parent"] == el, "start_object"]
    expect_identical(index_find_id_bisect(rdsi, at3[[1]], el),
                     index_find_id_linear(rdsi, at3[[1]], el))
  }
})
