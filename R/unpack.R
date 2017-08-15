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

unpack_extract_plan <- function(index, id) {
  .Call(Cunpack_extract_plan, index, id)
}

to_sexptype <- function(x) {
  .Call(Cto_sexptype, x)
}

unpack_extract <- function(x, index, id, reuse_ref = FALSE) {
  .Call(Cunpack_extract, x, index, id, reuse_ref)
}

unpack_index_refs <- function(index) {
  .Call(Cunpack_index_refs, index)
}

unpack_index_refs_clear <- function(index) {
  .Call(Cunpack_index_refs_clear, index)
  invisible()
}

unpack_extract_element <- function(x, index, id, i, error_if_missing = TRUE) {
  .Call(Cunpack_extract_element, x, index, id, i, error_if_missing)
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
index_find_nth_child <- function(index, id, n) {
  .Call(Cindex_find_nth_child, index, id, n)
}
index_find_attribute <- function(x, index, id, name) {
  .Call(Cindex_find_attribute, x, index, id, name)
}
