mcJSON
======

[![Travis Build Status](https://travis-ci.org/FSMaxB/mcJSON.svg?branch=master)](https://travis-ci.org/FSMaxB/mcJSON)
[![Coverity Scan Build](https://scan.coverity.com/projects/6759/badge.svg)](https://scan.coverity.com/projects/6759)

mcJSON is a Fork of [cJSON](http://sourceforge.net/projects/cjson/) with the goal of making it fit for creating JSON containing cryptographic keys and other confidential data without leaking this confidential data.

**THE API ISN'T CONSIDERED STABLE, SO KEEP IN MIND IT MAY CHANGE AT ANY MOMENT, ALSO, MCJSON_UTILS HASN'T BEEN PORTED YET.**

Features/Differences in comparison with cJSON:
* New coding style (cJSONs source code looks quite ugly!)
* No null terminated c strings, mcJSON uses [molch-buffer](https://github.com/FSMaxB/molch-buffer) instead
* Allocated memory is zeroed after use **(currently only applies to strings, not numbers, bools etc.)**
* Depends on [libsodium](https://github.com/jedisct1/libsodium) (may change in the future)
* Doesn't contain stupid mistakes that even a compiler with proper flags can catch. (It still contains mistakes though, every piece of software does)
* Buffered parsing (allocate one big continuous chunk of memory and parse the entire JSON tree into it)
* Arrays and objects know their own length (getting the size has O(1) complexity )

mcJSON is licensed under the ISC license.
