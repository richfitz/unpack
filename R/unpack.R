## wrapper
unpack_all <- function(x) {
  .Call(Cunpack_all, x)
}

## rdsi
rdsi_build <- function(x) {
  .Call(Crdsi_build, x)
}

rdsi_get_index_matrix <- function(rdsi) {
  .Call(Crdsi_get_index_matrix, rdsi)
}

rdsi_get_data <- function(rdsi) {
  .Call(Crdsi_get_data, rdsi)
}

rdsi_get_refs <- function(rdsi) {
  .Call(Crdsi_get_refs, rdsi)
}

rdsi_del_refs <- function(rdsi) {
  .Call(Crdsi_del_refs, rdsi)
}

## extract
unpack_extract_plan <- function(rdsi, id) {
  .Call(Cunpack_extract_plan, rdsi, id)
}

unpack_extract <- function(rdsi, id, reuse_ref = FALSE) {
  .Call(Cunpack_extract, rdsi, id, reuse_ref)
}


## helpers
to_sexptype <- function(x) {
  .Call(Cto_sexptype, x)
}

## OLD:

## extract plan


unpack_index_refs <- function(index) {
  .Call(Cunpack_index_refs, index)
}

unpack_index_refs_clear <- function(index) {
  .Call(Cunpack_index_refs_clear, index)
  invisible()
}

## Find
index_find_element <- function(index, id, i) {
  .Call(Cindex_find_element, index, id, i)
}
index_find_nth_child <- function(index, id, n) {
  .Call(Cindex_find_nth_child, index, id, n)
}
index_find_car <- function(index, id) {
  .Call(Cindex_find_car, index, id)
}
index_find_cdr <- function(index, id) {
  .Call(Cindex_find_cdr, index, id)
}
index_find_id <- function(index, at, start_id = 0L) {
  .Call(Cindex_find_id, index, at, start_id)
}
index_find_attributes <- function(index, id) {
  .Call(Cindex_find_attributes, index, id)
}

## Below here uses both the data and the index; so we'll treat them
## differently.

## Search
index_search_attribute <- function(x, index, id, name) {
  .Call(Cindex_search_attribute, x, index, id, name)
}
index_search_character <- function(x, index, id, name) {
  .Call(Cindex_search_character, x, index, id, name)
}

## Pick
unpack_pick_attributes <- function(x, index, id = 0L, reuse_ref = FALSE) {
  .Call(Cunpack_pick_attributes, x, index, id, reuse_ref)
}
