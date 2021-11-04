# Building on Windows 10
Windows is not a target platform for this project, but it is possible to build space-expansion on windows. This guide describes how to build space-expansion server on windows 10. Probably, you may use it to build server on windows 7 or 8 too.

## Installing required tools
1. Install **Build Tools for Visual Studio 2019** from [this page](https://visualstudio.microsoft.com/ru/downloads/) or use this [direct link](https://visualstudio.microsoft.com/ru/thank-you-downloading-visual-studio/?sku=BuildTools&rel=16); 
2. Install [CMake](https://cmake.org/download/);
   It will be better, if you select the "Add CMake to system path" option during the installation process;
3. Install [Python](https://www.python.org/downloads/);
   It would be better if you select the "Add Python3.8 to PATH" option during the installation process.
4. install [Git](https://git-scm.com/)
5. install Conan packet manager (see below)

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

## Prepharing conan (optional)
This step can be skipped, but it can be usefull if you encountered some error and trying to figure out what was wrong.
Conan profile specifies, which compiler, bitness, options and other significant parameters will be used to build dependencies. For more details see the official ["Conan profiles"](https://docs.conan.io/en/latest/reference/profiles.html) page.

If you have already run conan, you may want to remove the conan cache first. It can be done with the following command:
```
Remove-Item –path $HOME/.conan –recurse
```
Now, let's create default profile:
```
conan profile new default --detect

Found Visual Studio 16
Profile created with detected settings: C:\Users\zimin\.conan\profiles\default
```
In this case, conan detect only Visual Studio 16 compiler. If you have installed other compilers, conan may find them too. Now let's take a look at profile:
```
notepad.exe $HOME/.conan/profiles/default
```

You may see the following lines:
```
[settings]
os=Windows
os_build=Windows
arch=x86_64
arch_build=x86_64
compiler=Visual Studio
compiler.version=16
build_type=Release
[options]
[build_requires]
[env]
```
This means, that Visual Studio compiler will be used (version 16) to build dependencies in Release 64-bit mode. That is what we need. Server should also be built with the same compiler, bitness and build type (Release).

**In general**, if you have some error and suspect that it is because something wrong with conan, you can clear conan cache, check conan profiile and rebuild all dependencies again

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

Clone the server's sources and switch to `release-1.0` branch:
```powershell
git clone https://github.com/ziminas1990/space-expansion.git $SPEX_SOURCE_DIR
git checkout release-1.0
```

Create build directory and move into it:
```
mkdir $SPEX_BUILD_DIR
cd $SPEX_BUILD_DIR
```

Add `bincrafters` remotes for Conan:
```
conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan
```

Run conan to build all required dependencies:
```powershell
conan install $SPEX_SOURCE_DIR/server/conanfile.txt --build=missing
```
**Hint:** after you have run conan, you may want to check if it used proper compiler and build configuration. Please, refer to "Conan profile" for details.

Start building with cmake:
```
cmake $SPEX_SOURCE_DIR/server
cmake --build . --config Release
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
