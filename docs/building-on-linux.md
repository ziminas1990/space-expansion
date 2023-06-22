
# Building on Linux
This manual describes building on Ubuntu 23.04.

## Preparing system
In this article let's assume that you have the following environment variables:
```bash
SPEX_SOURCE_DIR=$HOME/dev/space-expansion
SPEX_BUILD_DIR=$HOME/dev/space-expansion-build
SPEX_VENV_DIR=$HOME/dev/space-expansion-venv
```
Feel free to specify another paths.

Install the following packages:
```bash
sudo apt install cmake git python3 python3-pip python3-venv
```

Create python virtual environment and install Conan:
```bash
python3 -m venv $SPEX_VENV_DIR
source $SPEX_VENV_DIR/bin/activate
pip3 install conan
```

**Selfcheck:** make sure Conan can be run:
```bash
$ conan --version
Conan version 2.0.6
```

## Preparing Conan
This step may be skipped, but it is highly recommended to do it attentively.

Conan profile specifies which compiler, bitness, options and other significant parameters will be used to build the dependencies. For more details see the official ["Conan profiles"](https://docs.conan.io/en/latest/reference/profiles.html) page.

If you have already run Conan, you may want to remove the Conan cache first. It can be done with the following command:
```bash
rm -rf $HOME/.conan
```
Now, let's create a default profile:
```bash
conan profile detect
```
It will print detected environment and you should get something similar to:
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
This means, that bt default a gcc 12 compiler will be used to build dependencies in Release 64-bit mode.

**In general**, if you have some error and suspect that it is because something wrong with Conan, you can clear Conan cache, check Conan profile and rebuild all dependencies again

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

Now, to build release build run the following commands:
```bash
# Building dependencies
conan install $SPEX_SOURCE_DIR/server/conanfile.txt --output-folder=$SPEX_BUILD_DIR --build=missing
# Building server
cmake -S $SPEX_SOURCE_DIR/server -B $SPEX_BUILD_DIR --preset conan-release
cmake --build $SPEX_BUILD_DIR --config Release -- -j6
```

If you want to build development build with autotests, run the following commands:
```bash
# Building dependencies
conan install $SPEX_SOURCE_DIR/server/conanfile.txt --output-folder=$SPEX_BUILD_DIR --build=missing -s build_type=Debug
# Building server
cmake -S $SPEX_SOURCE_DIR/server -B $SPEX_BUILD_DIR -Dautotests-mode=ON -Dbuild-debug=ON --preset conan-debug
cmake --build $SPEX_BUILD_DIR --config Debug -- -j6
```

If you want to force 32-bit build, you should:
1. add `-s arch=x86` to the `conan install` command, to build all dependencies in 32-bit mode;
2. add `-Dbuild-32bit=ON` to the first cmake command, to configure 32-bit build.

## Run integration tests
Install additional dependencies to your python venv:
```bash
source $SPEX_VENV_DIR/bin/activate
pip install protobuf pyaml
```

To run tests execute the following script:
```bash
# Directory with the server's executable
export PATH=$PATH:$SPEX_BUILD_DIR/bin
export PYTHONPATH=$SPEX_SOURCE_DIR/python-sdk
cd $SPEX_SOURCE_DIR/tests
python -m unittest discover
```
