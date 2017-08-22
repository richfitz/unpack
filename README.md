# unpack

[![Project Status: WIP - Initial development is in progress, but there has not yet been a stable, usable release suitable for the public.](http://www.repostatus.org/badges/latest/wip.svg)](http://www.repostatus.org/#wip)
[![Linux Build Status](https://travis-ci.org/richfitz/unpack.svg?branch=master)](https://travis-ci.org/richfitz/unpack)
[![Windows Build status](https://ci.appveyor.com/api/projects/status/github/richfitz/unpack?svg=true)](https://ci.appveyor.com/project/richfitz/unpack)
[![codecov.io](https://codecov.io/github/richfitz/unpack/coverage.svg?branch=master)](https://codecov.io/github/richfitz/unpack?branch=master)

Unpack a serialized R (RDS) file into its constituent parts

### Design

The package implements a number of related things

* `unpack` - a ground-up rewrite of `unserialize`.  It will not be any faster
* The idea of an **index** for an rds file.  This makes subsequent lookups very quick.  Use `index_rds` to build the index
* extract a given element by index, name, etc
* A new file format `rdsi` which includes this index at save time, and functions `read_rdsi` and `save_rdsi` (analagous to `readRDS` and `writeRDS`).

The package does not try to work around any limitations of the rds format.  To build the index, one must traverse the entire contents of the rds data; in particular attributes of the first object are stored at the *very end* of the rds and to find out where they are we must traverse through the entire contents!

### Limitations

* For reading off disk, the entire rds file must be loaded into memory.  So to determine the class of a 200MB long rds file you will consume 200MB of memory and have to read the whole contents off disk.  Sorry.  Working around this means understanding the dire warnings in `?seek` about windows as we do need some degree of random access to the underlying data.
* Because of the above, and because `readRDS` from disk is typically I/O bound, we will need to implement some form of support for reading from file.  We do need to dash about within the file a bit, so memory mapping the file might be one way forward.  Boost provides one such platform-independent abstraction
  - this really needs implementing to make the package actually useful
  - the i/o is done via buffer.c and non-array based version could be done here
* Does not support reference hook functions, though that could be implemented (especially if I knew what the use case was).  Pointers behave as badly as usual with serialisation.
* Does not support ASCII serialisation.  I'm not sure that I want to support this, but it would not be the end of the world to do.
* Long vector support is patchy - there is lots of testing required to get right, and there are non-long types floating around

### Use cases

* reading in large objects just to see what class they are, how long they are, etc is memory intensive; grabbing just what we need might be nicer
* we are not I/O bound in cases where raw vectors are passed back directly.  Examples include databases, web apis, etc; in these cases the requirement to fully read in the object moot because this is already done
* as a special case of the above [thor](https://github.com/richfitz/thor) will pass back a memory-mapped file for the object, saving any copies at all


### Prior work:

* discussed [in this issue](https://github.com/ropensci/unconf/issues/37) from the [2015 ropensci unconf](http://unconf.ropensci.org)
