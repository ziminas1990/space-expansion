# Building on Windows 10
Windows is not a target platform for this project, but it is possible to build space-expansion on windows. This guide describes how to build space-expansion server on windows 10. Probably, you may use it to build server on windows 7 or 8 too.

## Installing required tools
1. Install **Build Tools for Visual Studio 2022** from [this page](https://visualstudio.microsoft.com/ru/downloads/)
2. ~~Install **LLVM** for windows from this page: [LLVM Download](https://releases.llvm.org/download.html)~~
3. Install [CMake](https://cmake.org/download/)
   It will be better, if you select the "Add CMake to system path" option during the installation process
4. Install [Python](https://www.python.org/downloads/)
   It would be better if you select the "Add Python3.8 to PATH" option during the installation process
5. install [Git](https://git-scm.com/)
6. install Conan packet manager (see below)

After all this packages are installed, make sure that you can run them from the command line interface (CLI). To start CLI press **"WIN + R"** and type the **"powershell"** command.

To check that tools are accessable from the CLI, run the following commands to see the similar output:
```powershell
cmake --version

cmake version 3.17.0
CMake suite maintained and supported by Kitware (kitware.com/cmake).
```
```powershell
git --version

git version 2.25.1.windows.1
```
```powershell
python --version

Python 3.8.2
```
```powershell
pip3 --version

pip 19.2.3 from c:\users\zimin\appdata\local\programs\python\python38-32\lib\site-packages\pip (python 3.8)
```

If some of this command can't be found by windows you should add path to the corresponding application to the PATH environment variable (it also can be optionally done during installiation process).

If everything work as expected, you may want to install **Conan** packet manager - a powerfull tool for building dependencies for C++ projects. To install conan run:
```powershell
pip install conan
```

And finally, let's check that conan has been installed:
```powershell
conan --version

Conan version 1.23.0
```

## Prepharing conan
If you have already run conan, you may want to remove the conan cache first. It can be done with the following command:
```powershell
Remove-Item –path $HOME/.conan –recurse
```

Now, let's create default profile:
```powershell
conan profile detect

Found msvc 17
```
This will create a `default` conan profile at `$HOME/.conan/profiles/default`. As you may see, by default Conan is going to use MSVC.

**In general**, if you have some error and suspect that it is because something wrong with conan, you can clear conan cache, check conan profile and rebuild all dependencies again.

## Building server
Let's assume, that you have the following powershell variables:
1. **SPEX_SOURCE_DIR** - path to the directory, where you have cloned the latest [server sources](https://github.com/ziminas1990/space-expansion);
2. **SPEX_BUILD_DIR** - path to the directory, where you are going to build server.
Note that the *SPEX_SOURCE_DIR* and *SPEX_BUILD_DIR* shouldn't be the same directories!

For example, you can initialize them as follow:
```powershell
$SPEX_SOURCE_DIR="$HOME\Projects\space-expansion"
$SPEX_BUILD_DIR="$HOME\Projects\space-expansion-build"
```

Clone the server's sources and run Conan to install all required dependencies:
```powershell
git clone https://github.com/ziminas1990/space-expansion.git $SPEX_SOURCE_DIR
mkdir $SPEX_BUILD_DIR
conan install $SPEX_SOURCE_DIR/server/conanfile.txt --output-folder=$SPEX_BUILD_DIR --build=missing
```

Start building with cmake:
```powershell
cmake -S $SPEX_SOURCE_DIR/server -B $SPEX_BUILD_DIR --preset conan-default
cmake --build $SPEX_BUILD_DIR --config Release
```

# Troubleshooting
## CMake
When you run cmake configuration for the first time, it should also the similar log:
```powershell
-- Building for: Visual Studio 16 2019
-- Selecting Windows SDK version 10.0.18362.0 to target Windows 10.0.18363.
-- The C compiler identification is MSVC 19.25.28610.4
-- The CXX compiler identification is MSVC 19.25.28610.4
```
**Make sure** that both cmake and conan are using the same compiler!

# Prepare python SDK
This step is required in case you are going to use Python SDK or run integration tests.

Create python virtual environment for space-expansion:
```
$SPEX_VENV_DIR="$HOME\Projects\space-expansion-venv"
python -m venv create $SPEX_VENV_DIR
```

In order to run `activate.ps1` you might need to set up execution policy for the current user:
```
Set-ExecutionPolicy -ExecutionPolicy Unrestricted -Scope CurrentUser
```

Activating virtual environment:
```
cd $SPEX_VENV_DIR
Scripts\Activate
```

Install required dependencies:
```
pip install pyyaml protobuf==3.20.0
```
**NOTE:** please, make sure that protobuf package version matches (or close to) the protobuf version, specified in `conanfile.txt` ($SPEX_SOURCE_DIR/server/conanfile.txt)!

# Run integration tests
Set up environment:
```
$env:PYTHONPATH="$SPEX_SOURCE_DIR\python-sdk"
$env:SPEX_SERVER_BINARY="$SPEX_BUILD_DIR\Release\space-expansion-server.exe"
```
**Note:** please, make sure that `$env:SPEX_SERVER_BINARY` contains real binary path.

Finally, run the tests:
```
cd $SPEX_SOURCE_DIR\tests
python -m unittest
```
