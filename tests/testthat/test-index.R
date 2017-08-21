context("index")

## This case is the easiest
test_that("null", {
  xb <- serialize_binary(NULL)
  expect_equal(length(xb), 18L)

  idx <- unpack_index(xb)
  expect_equal(nrow(idx), 1)
  expect_equal(attr(idx, "n_refs"), 0L)
  expect_equal(attr(idx, "len_data"), 18L)
  idx <- idx[1, ]

  expect_identical(idx[["id"]], 0L)
  expect_identical(idx[["parent"]], 0L)
  expect_identical(idx[["type"]], sexptypes[["NILVALUE"]])
  expect_identical(idx[["length"]], 0L)
  expect_identical(idx[["refid"]], 0L)
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

test_that("attributed attributes", {
  x <- structure(pi, a = 1, bar = 2)
  v <- structure("hello", x = x, y = TRUE)
  idx <- unpack_index(serialize_binary(v))
  ## TODO: what did I want to test here?
  idx[6, ]
})

test_that("namespace", {
  a <- asNamespace("stats")
  x <- serialize_binary(a)
  idx <- unpack_index(x)
  expect_equal(nrow(idx), 3)
  expect_equal(
    rawToChar(x[(idx[2, "start_data"] + 1):(idx[2, "start_attr"])]),
    "stats")
  expect_equal(
    rawToChar(x[(idx[3, "start_data"] + 1):(idx[3, "start_attr"])]),
    as.character(getRversion()))
})

test_that("package", {
  p <- as.environment("package:stats")
  x <- suppressWarnings(serialize_binary(p))
  idx <- unpack_index(x)
  expect_equal(nrow(idx), 2)
  expect_equal(
    rawToChar(x[(idx[2, "start_data"] + 1):(idx[2, "start_attr"])]),
    "package:stats")
})

test_that("external pointer", {
  b <- as.raw(c(0x42, 0x0a, 0x02, 0x00, 0x00, 0x00, 0x00, 0x04, 0x03,
                0x00, 0x00, 0x03, 0x02, 0x00, 0x16, 0x00, 0x00, 0x00,
                0xfe, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00))
  idx <- unpack_index(b)
  expect_equal(nrow(idx), 3)
  expect_equal(idx[, "type"],
               unname(sexptypes[c("EXTPTRSXP", "NILVALUE", "NILVALUE")]))
})

test_that("environment", {
  ## Easiest to start with; an environment with one thing in it, and
  ## no hash table.
  e <- new.env(parent = emptyenv(), hash = FALSE)
  e$a <- 1L
  idx <- unpack_index(serialize_binary(e))
  invisible(unpack_all(serialize_binary(e)))

  expect_equal(idx[, "type"][[1]], sexptypes[["ENVSXP"]])
  expect_equal(idx[, "length"][[1]], 1L)
  expect_equal(idx[, "refid"][[1]], 1L)

  ## Four children:
  i <- idx[, "parent"] == 0 & idx[, "id"] != 0
  expect_equal(sum(i), 4)
  expect_equal(to_sexptype(idx[i, "type"]),
               ## enclos,     frame,     hashtab,    attrib
               c("EMPTYENV", "LISTSXP", "NILVALUE", "NILVALUE"))

  ## So all the action is happening in the second child, which is a listsxp
  ##
  ## We have listsxp, with an attribute; that's the name (via a
  ## symbol), and the value as the CAR, NULL as the CDR
  j <- idx[, "parent"] == idx[i, "id"][[2]]
  idx[j, ]
  expect_equal(unname(idx[j, "type"]),
               unname(sexptypes[c("SYMSXP", "INTSXP", "NILVALUE")]))
})

test_that("symbol reference", {
  ## Here's a fairly straightforward case, contrived to make the
  ## second list element contain an object that contains one backward
  ## reference and one internal reference.
  a <- list(c(a = 1), list(structure(c(x = 2), class = "foo"),
                           structure(c(y = 3), class = "bar")))
  x <- serialize_binary(a)
  rdsi <- rdsi_build(x)
  expect_null(rdsi_get_refs(rdsi))

  idx <- rdsi_get_index_matrix(rdsi)

  res <- unpack_extract_plan(rdsi, 8L)
  expect_equal(res, c(3L, NA_integer_))
  expect_equal(to_sexptype(idx[4, "type"]), "SYMSXP")

  res <- unpack_extract(rdsi, 8L)
  expect_identical(res, a[[2]])

  expect_null(rdsi_get_refs(rdsi))

  ## Then again, but with some reference reuse
  res2 <- unpack_extract(rdsi, 8L, reuse_ref = TRUE)
  expect_identical(res, res2)
  expect_identical(rdsi_get_refs(rdsi),
                   list(quote(names), quote(class)))

  res3 <- unpack_extract(rdsi, 8L, reuse_ref = TRUE)
  res4 <- unpack_extract(rdsi, 8L, reuse_ref = FALSE)
  expect_identical(res, res3)
  expect_identical(res, res4)
  expect_identical(rdsi_get_refs(rdsi),
                   list(quote(names), quote(class)))

  rdsi_del_refs(rdsi)
  expect_null(rdsi_get_refs(rdsi))
})

## Try to manufacture a recursive reference case.  If we have a series
## of forward references we can get this to happen
test_that("recursive", {
  e1 <- new.env(parent = emptyenv(), hash = FALSE)
  e1$a <- 1
  e2 <- new.env(parent = e1, hash = FALSE)
  e2$a <- 2
  e3 <- new.env(parent = e2, hash = FALSE)
  e3$a <- 3
  x <- serialize_binary(list(e1, e2, e3))
  rdsi <- rdsi_build(x)
  expect_null(rdsi_get_refs(rdsi))

  idx <- rdsi_get_index_matrix(rdsi)

  res <- unpack_all(x)

  i <- which(idx[, "refid"] != 0 & idx[, "type"] != sexptypes[["REFSXP"]])
  expect_equal(idx[i, "refid"], 1:4)
  expect_equal(idx[i, "type"],
               unname(sexptypes[c("ENVSXP", "SYMSXP", "ENVSXP", "ENVSXP")]))

  expect_identical(unpack_extract(rdsi, i[[2]] - 1L), as.name("a"))

  ## Then start work on the different environments.  Outermost is easiest:
  res <- unpack_extract(rdsi, i[[1]] - 1L)
  expect_is(res, "environment")
  expect_identical(parent.env(res), emptyenv())
  expect_identical(res$a, 1)

  ## Second contains one backward reference
  res <- unpack_extract(rdsi, i[[3]] - 1L)
  expect_is(res, "environment")
  expect_identical(res$a, 2)
  res_p <- parent.env(res)
  expect_identical(parent.env(res_p), emptyenv())
  expect_identical(res_p$a, 1)

  ## Third contains a recursive backward reference
  res <- unpack_extract(rdsi, i[[4]] - 1L)
  expect_is(res, "environment")
  expect_identical(res$a, 3)
  res_p1 <- parent.env(res)
  expect_identical(res_p1$a, 2)
  res_p2 <- parent.env(res_p1)
  expect_identical(parent.env(res_p2), emptyenv())
  expect_identical(res_p2$a, 1)

  ## Then some reference reuse:
  rdsi_del_refs(rdsi)
  expect_null(rdsi_get_refs(rdsi))

  res3 <- unpack_extract(rdsi, i[[4]] - 1L, reuse_ref = TRUE)
  refs <- rdsi_get_refs(rdsi)
  expect_identical(refs[[4]], res3)

  res2 <- unpack_extract(rdsi, i[[3]] - 1L, reuse_ref = TRUE)
  res1 <- unpack_extract(rdsi, i[[1]] - 1L, reuse_ref = TRUE)

  expect_identical(emptyenv(), parent.env(res1))
  expect_identical(res1, parent.env(res2))
  expect_identical(res2, parent.env(res3))

  ## And this is _precicely_ where things get really dangerous;
  ## modifications after extraction are mirrored through the
  ## subsequent extractions.
  res1$x <- pi
  expect_identical(parent.env(res2)$x, pi)
  res2_2 <- unpack_extract(rdsi, i[[3]] - 1L, reuse_ref = TRUE)
  expect_identical(parent.env(res2_2)$x, pi)

  ## Can still get a fresh copy though
  res2_3 <- unpack_extract(rdsi, i[[3]] - 1L, reuse_ref = FALSE)
  expect_null(parent.env(res2_3)$x)

  ## But then we can continue:
  expect_identical(unpack_extract(rdsi, i[[3]] - 1L, reuse_ref = TRUE),
                   res2)
})
