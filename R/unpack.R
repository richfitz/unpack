unpack_all <- function(x) {
  .Call(Cunpack_all, x)
}

unpack_inspect <- function(x) {
  .Call(Cunpack_inspect, x)
}

unpack_index <- function(x) {
  .Call(Cunpack_index, x)
}

to_sexptype <- function(x) {
  .Call(Cto_sexptype, x)
}
