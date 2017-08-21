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
