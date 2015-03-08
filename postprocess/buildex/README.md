buildex
======

This is the main analysis project which is responsible for analyzing the instraces and memdumps that are generated as the last phase of DynamoRIO instrumentation.

building the project
-------------------

Inside the utility folder,

1. Run build.bat {m32/m64} {debug/release}
2. e.g:- if you build a 32 bit debug build by running *build.bat m32 debug*, a folder build32 will be created with the VS2013 solution. 
Further, it would build the solution as well.

For specifics of how to contribute to the project, please refer individual src directories.