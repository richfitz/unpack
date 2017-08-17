context("pick")

test_that("simple case", {
  x <- 1:5
  attr(x, "class") <- "mything"
  names(x) <- c("one", "two", "three", "four", "five")
  xb <- serialize_binary(x)
  idx_ptr <- unpack_index(xb, TRUE)

  expect_identical(unpack_pick_attributes(xb, idx_ptr),
                   as.pairlist(attributes(x)))
})

## next up - start with an object that combines x and idx so we can
## make it immutable
