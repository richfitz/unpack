context("pick")

test_that("simple case", {
  x <- 1:5
  attr(x, "class") <- "mything"
  names(x) <- c("one", "two", "three", "four", "five")
  xb <- serialize_binary(x)
  rdsi <- rdsi_build(xb)

  expect_identical(unpack_pick_attributes(rdsi),
                   as.pairlist(attributes(x)))
  expect_identical(unpack_pick_attribute(rdsi, "class"), class(x))
  expect_identical(unpack_pick_class(rdsi), class(x))
  expect_identical(unpack_pick_names(rdsi), names(x))
  expect_null(unpack_pick_dim(rdsi))
  expect_equal(unpack_pick_typeof(rdsi), typeof(x))

  expect_null(unpack_pick_attribute(rdsi, "class", 1L))
  expect_identical(unpack_pick_class(rdsi, 1L), "pairlist")
  expect_identical(unpack_pick_typeof(rdsi, 1L), "pairlist")
})

test_that("dim: vector", {
  v <- 1:10
  rdsi <- rdsi_build(serialize_binary(v))
  expect_identical(unpack_pick_length(rdsi), 10L)
  expect_null(unpack_pick_dim(rdsi))
})

test_that("dim: matrix", {
  m <- matrix(runif(35), 5, 7)
  rdsi <- rdsi_build(serialize_binary(m))
  expect_identical(unpack_pick_length(rdsi), length(m))
  expect_identical(unpack_pick_dim(rdsi), dim(m))
})

test_that("dim: array", {
  a <- array(runif(3 * 5 * 7), c(3, 5, 7))
  rdsi <- rdsi_build(serialize_binary(a))
  expect_identical(unpack_pick_length(rdsi), length(a))
  expect_identical(unpack_pick_dim(rdsi), dim(a))
})

test_that("dim: data.frame - no row.names", {
  df <- iris
  rdsi <- rdsi_build(serialize_binary(df))
  idx <- rdsi_get_index_matrix(rdsi)
  expect_identical(unpack_pick_length(rdsi), length(df))
  expect_identical(unpack_pick_dim(rdsi), dim(df))
})

test_that("dim: data.frame - with row.names", {
  df <- mtcars
  rdsi <- rdsi_build(serialize_binary(df))
  idx <- rdsi_get_index_matrix(rdsi)
  expect_identical(unpack_pick_length(rdsi), length(df))
  expect_identical(unpack_pick_dim(rdsi), dim(df))
})
