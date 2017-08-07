unpack_all <- function(x) {
  .Call(Cunpack_all, x)
}

unpack_inspect <- function(x) {
  .Call(Cunpack_inspect, x)
}

unpack_index <- function(x, as_ptr = FALSE) {
  .Call(Cunpack_index, x, as_ptr)
}

unpack_index_as_matrix <- function(x) {
  .Call(Cunpack_index_as_matrix, x)
}

to_sexptype <- function(x) {
  .Call(Cto_sexptype, x)
}

unpack_extract <- function(x, index, id) {
  .Call(Cunpack_extract, x, index, id)
}
