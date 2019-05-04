# space-expansion-server
## Building
This project supports building only with CMake + conan.
The easiest way to build application is to run at the build directory the following commands:
```
SOURCE_DIR=/path/to/sources
conan install $SOURCE_DIR/conanfile.txt
cmake $SOURCE_DIR
cmake --build . -- -j4
```
