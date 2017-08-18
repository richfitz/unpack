## wrapper
unpack_all <- function(x) {
  .Call(Cunpack_all, x)
}

## rdsi
rdsi_build <- function(obj) {
  .Call(Crdsi_build, obj)
}

rdsi_get_index_matrix <- function(obj) {
  .Call(Crdsi_get_index_matrix, obj)
}

rdsi_get_data <- function(obj) {
  .Call(Crdsi_get_data, obj)
}

rdsi_get_refs <- function(obj) {
  .Call(Crdsi_get_refs, obj)
}

rdsi_del_refs <- function(obj) {
  .Call(Crdsi_del_refs, obj)
}

## helpers
to_sexptype <- function(x) {
  .Call(Cto_sexptype, x)
}

## OLD:

## extract plan
unpack_extract_plan <- function(index, id) {
  .Call(Cunpack_extract_plan, index, id)
}


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
unpack_extract <- function(x, index, id, reuse_ref = FALSE) {
  .Call(Cunpack_extract, x, index, id, reuse_ref)
}

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
