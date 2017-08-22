context("unpack_df")

test_that("create", {
  dat <- mtcars
  rdsi <- rdsi_build(serialize_binary(dat))

  expect_error(unpack_df_create(rdsi, 1L),
               "is not a data.frame")
  df <- unpack_df_create(rdsi, 0L)

})
