# space-expansion-server
## References
  * [Youtrack](https://space-expansion.myjetbrains.com/youtrack/issues?q=project:%20space-expansion-server)

## Docs
  - [API Description (rus)](Doc/API.ru.md)

## What others tell about us
```А код у тебя чистый, понятный, GJ``` - [Anton SX91](https://github.com/SX91)

## Building
This project supports building only with CMake + conan.
The easiest way to build application is to run at the build directory the following commands:
```
SOURCE_DIR=/path/to/sources
conan install $SOURCE_DIR/conanfile.txt --build=missing
cmake $SOURCE_DIR
cmake --build . -- -j4
```

If you are going to have development build configuration (with autotests) you should run:
```
SOURCE_DIR=/path/to/sources
conan install $SOURCE_DIR/conanfile.txt --build=missing -s build_type=Debug
cmake $SOURCE_DIR -Dautotests-mode=ON -Dbuild-debug=ON
cmake --build . -- -j4
```

To force 32-bit development build you should run:
```
SOURCE_DIR=/path/to/sources
conan install $SOURCE_DIR/conanfile.txt --build=missing -s build_type=Debug -s arch=x86
cmake $SOURCE_DIR -Dautotests-mode=ON -Dbuild-debug=ON -Dbuild-32bit=ON
cmake --build . -- -j4
```
