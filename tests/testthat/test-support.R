context("support")

test_that("sexptype is ordered", {
  expect_equal(sexptypes, sort(sexptypes))
})

test_that("can recover all sexptypes", {
  expect_equal(to_sexptype(sexptypes), names(sexptypes))

  cmp <- rep(NA_character_, 255)
  cmp[sexptypes] <- names(sexptypes)
  expect_identical(cmp, to_sexptype(1:255))
})
