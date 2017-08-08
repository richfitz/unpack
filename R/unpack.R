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

index_find_id <- function(index, at, start_id = 0L) {
  .Call(Cindex_find_id, index, at, start_id)
}
index_find_attributes <- function(index, id) {
  .Call(Cindex_find_attributes, index, id)
}
index_find_car <- function(index, id) {
  .Call(Cindex_find_car, index, id)
}
index_find_cdr <- function(index, id) {
  .Call(Cindex_find_cdr, index, id)
}
index_find_nth_daughter <- function(index, id, n) {
  .Call(Cindex_find_nth_daughter, index, id, n)
}
index_find_attribute <- function(index, id, name, x) {
  .Call(Cindex_find_attribute, index, id, name, x)
}
