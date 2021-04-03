# Additional manual

## Outliers handling

In some situations, the attribute values can be polluted with some obvious outliers, that will decrease performance.
This software offers an optional outlier handling feature, that can be used to remove these before using the data
to train the tree.


## Missing data handling

Some datasets have missing attribute values.
This can be handled with this software.
All these features need to be enabled at build time, by defining the symbol `HANDLE_MISSING_VALUES`
(done with `make HMS=Y`)

If enabled, the class DataPoint has these member functions:
* `size_t nbMissingValues() const`
* `bool valueIsMissing( size_t idx ) const;`
* `bool isMissingValue( std::string str ) const;`


*/
