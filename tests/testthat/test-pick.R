context("pick")

test_that("simple case", {
  x <- 1:5
  attr(x, "class") <- "mything"
  names(x) <- c("one", "two", "three", "four", "five")
  xb <- serialize_binary(x)
  rdsi <- rdsi_build(xb)

  expect_identical(unpack_pick_attributes(rdsi),
                   as.pairlist(attributes(x)))
})

## next up - start with an object that combines x and idx so we can
## make it immutable
