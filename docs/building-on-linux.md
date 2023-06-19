
# Building on Linux
This manual describes building on linuex Ubuntu. Th Ubuntu 19.10 was used to write this guide.

## Prepharing system
In this article let's assume that you have the following environment variables:
```bash
SPEX_SOURCE_DIR=$HOME/dev/space-expansion
SPEX_BUILD_DIR=$HOME/dev/space-expansion-build
SPEX_VENV_DIR=$HOME/dev/space-expansion-venv
```
Feel free to specify another paths.

Install the following packages:
```bash
sudo apt install cmake git python3 python3-pip
```

Install conan:
```bash
sudo pip3 install conan
```
Note: you can install conan without sudo, but in this case you'll have to manually add conan to the your PATH environment.


## Prepharing conan
This step may be skipped, but it is highly recommended to do it attentively.

Conan profile specifies which compiler, bitness, options and other significant parameters will be used to build the dependencies. For more details see the official ["Conan profiles"](https://docs.conan.io/en/latest/reference/profiles.html) page.

If you have already run conan, you may want to remove the conan cache first. It can be done with the following command:
```
rm -rf $HOME/.conan
```
Now, let's create a default profile:
```
conan profile new default --detect

Found gcc 9
gcc>=5, using the major as version
```
In this case, conan detected only gcc 9 compiler. If you have installed other compilers, conan may find them too. Also conan may print a warning:
```bash
************************* WARNING: GCC OLD ABI COMPATIBILITY ***********************
```
It is highly recommended to fix it with the following command:
```
conan profile update settings.compiler.libcxx=libstdc++11 default
```

Now let's take a look at the profile:
```
cat $HOME/.conan/profiles/default
```

You may see the following lines:
```
[settings]
os=Linux
os_build=Linux
arch=x86_64
arch_build=x86_64
compiler=gcc
compiler.version=9
compiler.libcxx=libstdc++11
build_type=Release
[options]
[build_requires]
[env]
```
This means, that a gcc9 compiler will be used to build dependencies in Release 64-bit mode.

**In general**, if you have some error and suspect that it is because something wrong with conan, you can clear conan cache, check conan profile and rebuild all dependencies again

## Building server
Preparing to build:
```bash
# Clone the sources and swtich to stable branch
git clone git@github.com:ziminas1990/space-expansion.git $SPEX_SOURCE_DIR
cd $SPEX_SOURCE_DIR
git checkout stable
# Create build directory and move into it
mkdir $SPEX_BUILD_DIR
cd $SPEX_BUILD_DIR
```

Now, to build release build run the following commands:
```bash
# Building dependencies
conan install $SPEX_SOURCE_DIR/server/conanfile.txt --build=missing -s build_type=Release
# Building server
cmake $SPEX_SOURCE_DIR/server
cmake --build . -- -j6
```

If you want to build development build with autotests, run the following commands:
```bash
# Building dependencies
conan install $SPEX_SOURCE_DIR/server/conanfile.txt --build=missing -s build_type=Debug
# Building server
cmake $SPEX_SOURCE_DIR/server -Dautotests-mode=ON -Dbuild-debug=ON
cmake --build . -- -j6
```

If you want to force 32-bit build, you should:
1. add `-s arch=x86` to the `conan install` command, to build all dependencies in 32-bit mode;
2. add `-Dbuild-32bit=ON` to the first cmake command, to configure 32-bit build.

## Run integration tests
It is highly recommended to use [python venv](https://docs.python.org/3/library/venv.html)! Let's create and activate a new virtual environment with all required dependencies:
```bash
python3 -m venv $SPEX_VENV_DIR
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