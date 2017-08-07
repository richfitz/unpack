##' @useDynLib unpack, .registration = TRUE
sexptypes <- NULL
.onLoad <- function(...) {
  sexptypes <<- .Call(Csexptypes)
}
