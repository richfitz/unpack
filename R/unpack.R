unpack_all <- function(x) {
  .Call(Cunpack_all, x)
}

unpack_inspect <- function(x) {
  .Call(Cunpack_inspect, x)
}
