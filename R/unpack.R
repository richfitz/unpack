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
  invisible()
}

## extract
unpack_extract_plan <- function(rdsi, id) {
  .Call(Cunpack_extract_plan, rdsi, id)
}

unpack_extract <- function(rdsi, id, reuse_ref = FALSE) {
  .Call(Cunpack_extract, rdsi, id, reuse_ref)
}

## find
index_find_element <- function(rdsi, id, i) {
  .Call(Cindex_find_element, rdsi, id, i)
}
index_find_nth_child <- function(rdsi, id, n) {
  .Call(Cindex_find_nth_child, rdsi, id, n)
}
index_find_car <- function(rdsi, id) {
  .Call(Cindex_find_car, rdsi, id)
}
index_find_cdr <- function(rdsi, id) {
  .Call(Cindex_find_cdr, rdsi, id)
}
index_find_id_linear <- function(rdsi, at, start_id = 0L) {
  .Call(Cindex_find_id_linear, rdsi, at, start_id)
}
index_find_id_bisect <- function(rdsi, at, start_id = 0L) {
  .Call(Cindex_find_id_bisect, rdsi, at, start_id)
}
index_find_attributes <- function(rdsi, id) {
  .Call(Cindex_find_attributes, rdsi, id)
}

## Search
index_search_attribute <- function(rdsi, id, name) {
  .Call(Cindex_search_attribute, rdsi, id, name)
}
index_search_character <- function(rdsi, id, name) {
  .Call(Cindex_search_character, rdsi, id, name)
}
index_search_inherits <- function(rdsi, id, what) {
  .Call(Cindex_search_inherits, rdsi, id, what)
}

## Pick
unpack_pick_attributes <- function(rdsi, id = 0L, reuse_ref = FALSE) {
  .Call(Cunpack_pick_attributes, rdsi, id, reuse_ref)
}
unpack_pick_attribute <- function(rdsi, name, id = 0L, reuse_ref = FALSE) {
  .Call(Cunpack_pick_attribute, rdsi, name, id, reuse_ref)
}

## Some convenience functions
unpack_pick_class <- function(rdsi, id = 0L, reuse_ref = FALSE) {
  .Call(Cunpack_pick_class, rdsi, id, reuse_ref)
}
unpack_pick_dim <- function(rdsi, id = 0L, reuse_ref = FALSE) {
  unpack_pick_attribute(rdsi, "dim", id, reuse_ref)
}
unpack_pick_names <- function(rdsi, id = 0L, reuse_ref = FALSE) {
  unpack_pick_attribute(rdsi, "names", id, reuse_ref)
}
unpack_pick_typeof <- function(rdsi, id = 0L) {
  .Call(Cunpack_pick_typeof, rdsi, id)
}
unpack_pick_length <- function(rdsi, id = 0L) {
  .Call(Cunpack_pick_length, rdsi, id)
}
unpack_pick_dim <- function(rdsi, id = 0L, reuse_ref = FALSE) {
  .Call(Cunpack_pick_dim, rdsi, id, reuse_ref)
}

## helpers
to_sexptype <- function(x) {
  .Call(Cto_sexptype, x)
}

unpack_df_create <- function(rdsi, id = 0L) {
  .Call(Cunpack_df_create, rdsi, id)
}
