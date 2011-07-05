***************************************************************************
*  PROJECT:  libburrow C client API - JSON parser for HTTP backend.
*
*  AUTHOR:   See README in JSON_PARSER directory.
***************************************************************************

JSON_PARSER was chosen over a few other C-based parsers according to the following criteria:

* Relatively small size: 1 header, 1 implementation file.
* Provides callback mechanism. This might be useful since the C client API
  uses callbacks.
* Handles UTF16 surrogate pairs correctly and rejects singleton surrogates
  when transcoding to UTF8.
* parses on a per-byte basis. This allows for stream-parsing.


Inside /JSON_PARSER there is an already compiled executable 'jest'. This executable allows for simple testing of the parser via interactive input from the shell. To try it, just enter: ./jtest at the prompt.
i.e.: 

local:JSON_PARSER$ ./jtest
No locale provided, C locale is used
[1,2,"a","\"hello!\"","\"\u0041\""]
[
  integer: 1
  integer: 2
  string: 'a'
  string: '"hello!"'
  string: '"A"'
]

For more info / documentation look into the library itself. 


