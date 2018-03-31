### ISO 8601 Parsing

## Build instructions
* 1 - ``` ./make_deps.sh (requires bash based shell) ```
* 2 - ``` mkdir build ```
* 3 - ``` cd build ```
* 4 - ``` cmake .. ```
* 5 - ``` make ```

## Testing
From the build directory

# Compare performance to a generic parser method using date::parse
``` 
./benchmarks ./timestamps.txt ../javascipt_ts_test.txt
``` 
# Simple verifications
```
./iso8601_test
```

# Library Interface

Generic ISO 8601 Timestamp parser.  Can throw invalid_iso8601_timestamp if the format is unrecognized.
``` C++
constexpr std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> 
parse_iso8601_timestamp( std::string_view timestamp_str );
```

Restricted Javascript flavor of ISO8601 timestamps parser.  Will throw invalid_javascript_timestamp if it doesn't adhere to the format used by Javascript.
``` C++
constexpr std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> 
parse_javascript_timestamp( std::string_view timestamp_str );
```

