#CRS
Common Relation Structure refers to both the representation and the utility to translate database (RDBMS relation like) to different vendors.

The current CRS representation is a set of files for each relation which includes a meta file (INI), a content file that can be read with the help of the information from the meta file and additional meta files for variable-length datatype.

Run `make` to build it. Run `crs -h` to view usage.
