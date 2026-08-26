
# Building on Linux

This manual describes building on Ubuntu 23.04.

## Preparing system

In this article let's assume that you have the following environment variables:

```bash
SPEX_SOURCE_DIR=$HOME/dev/space-expansion
SPEX_BUILD_DIR=$HOME/dev/space-expansion-build
SPEX_VENV_DIR=$HOME/dev/space-expansion-venv
```

Feel free to specify other paths.

Install the following packages:

```bash
sudo apt install cmake git python3 python3-pip python3-venv pipx
pipx ensurepath
```

Create a Python virtual environment and install Conan:

```bash
pipx install conan
```

**Selfcheck:** make sure Conan can be run:

```bash
$ conan --version
Conan version 2.0.6
```

## Preparing Conan

This step may be skipped, but it is highly recommended to do it attentively.

A Conan profile specifies which compiler, bitness, options and other significant
parameters will be used to build the dependencies. For more details see the
official ["Conan profiles"](https://docs.conan.io/en/latest/reference/profiles.html) page.

If you have already run Conan, you may want to remove the Conan cache first. It can be done with the following command:

```bash
rm -rf $HOME/.conan
```

Now, let's create a default profile:

```bash
conan profile detect
```

It will print the detected environment and you should get something similar to:

```
Found gcc 12
gcc>=5, using the major as version
gcc C++ standard library: libstdc++11
Detected profile:
[settings]
arch=x86_64
build_type=Release
compiler=gcc
compiler.cppstd=gnu17
compiler.libcxx=libstdc++11
compiler.version=12
os=Linux
```
This means that by default a gcc 12 compiler will be used to build dependencies
in Release 64-bit mode.

**In general**, if you have an error and suspect that it is because something is
wrong with Conan, you can clear the Conan cache, check the Conan profile and
rebuild all dependencies again.

## Building server

Preparing to build:

```bash
# Clone the sources and swtich to stable branch
git clone git@github.com:ziminas1990/space-expansion.git $SPEX_SOURCE_DIR
cd $SPEX_SOURCE_DIR
# git checkout stable
# Create build directory and move into it
mkdir $SPEX_BUILD_DIR
```

## Release build

To make a release build, run the following commands:

```bash
# Building dependencies
conan install $SPEX_SOURCE_DIR/server/conanfile.txt --output-folder=$SPEX_BUILD_DIR --build=missing
# Building server
cmake -S $SPEX_SOURCE_DIR/server -B $SPEX_BUILD_DIR --preset conan-release
cmake --build $SPEX_BUILD_DIR --config Release -- -j$(nproc)
```

## Development build

A development build is not just a build with debug symbols and autotests. Its
configuration also provides important artifacts for the IDE, such as a
`compile_commands.json` file for Clangd.

To configure a development build, run the following commands:

```bash
# Building dependencies
conan install $SPEX_SOURCE_DIR/server/conanfile.txt --output-folder=$SPEX_BUILD_DIR --build=missing -s build_type=Debug

cmake -S $SPEX_SOURCE_DIR/server -B $SPEX_BUILD_DIR -Dbuild-debug=ON -Dwith-autotests=ON --preset conan-debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# compile_commands.json must be placed in source directory for Clangd to work
ln -s $SPEX_BUILD_DIR/compile_commands.json $SPEX_SOURCE_DIR/compile_commands.json
```

To build the server, run:

```bash
# Building server
cmake --build $SPEX_BUILD_DIR --config Debug -- -j$(nproc --ignore=1)
```

If you want to force a 32-bit build, you should:

1. add `-s arch=x86` to the `conan install` command, to build all dependencies
   in 32-bit mode;
2. add `-Dbuild-32bit=ON` to the first cmake command, to configure a 32-bit build.

To run autotests:

```bash
$SPEX_BUILD_DIR/autotests
```

To run the server, make sure that the `space-expansion.cfg` file exists in the working
directory, then run:

```bash
$SPEX_BUILD_DIR/space-expansion-server
```

## Run integration tests

Create and activate a Python virtual environment, then install the dependencies
required by the Python SDK:

```bash
python3 -m venv "$SPEX_VENV_DIR"
source "$SPEX_VENV_DIR/bin/activate"
python -m pip install pyyaml protobuf typing-extensions
```

To run the tests, execute the following script:

```bash
# Directory with the server's executable
export SPEX_SERVER_BINARY=$SPEX_BUILD_DIR/space-expansion-server
export PYTHONPATH=$SPEX_SOURCE_DIR/python-sdk
cd $SPEX_SOURCE_DIR/tests
python -m unittest discover
```
