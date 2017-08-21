xdr_read_int <- function(x) {
  .Call(Cxdr_read_int, x)
}
xdr_read_double <- function(x) {
  .Call(Cxdr_read_double, x)
}
xdr_read_complex <- function(x) {
  .Call(Cxdr_read_complex, x)
}
