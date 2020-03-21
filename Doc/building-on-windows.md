# Building in Windows 10
This article describes how to build space-expansion server on windows 10.

## Installing required tools
1. Install **Build Tools для Visual Studio 2019** from [this page](https://visualstudio.microsoft.com/ru/downloads/) or use this [direct link](https://visualstudio.microsoft.com/ru/thank-you-downloading-visual-studio/?sku=BuildTools&rel=16); 
2. Install [CMake](https://cmake.org/download/);
   It will be better, if you select the "Add CMake to system path" option during the installation process;
3. Install [Python](https://www.python.org/downloads/);
   It would be better if you select the "Add Python3.8 to PATH" option during the installation process.
4. install [Git](https://git-scm.com/)
5. install Conan packet manager (see below)

After all this packets are installed, make sure that you can run them from the command line interface (CLI). To start command line press **"WIN + R"** and type the **"powershell"** command.

To check that tools as accessable from the CLI, tun the following commands to see the similar output:
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

If some of this command can't be found by windows you should add path to corresponding application to the PATH environment variable (it also can be optionally done during installiation process).

If everything work as expected, you may want to install **Conan** packet manager is a powerfull tool for building dependencies for C++ projects. To install conan run:
```powershell
pip install conan
```

And finally, let's check that conan has been installed:
```powershell
conan --version

Conan version 1.23.0
```

## Building server
Let's assume, that you have the following powershell variables:
1. **SPEX_SOURCE_DIR** - path to the directory, where you have cloned the latest [server sources](https://github.com/ziminas1990/space-expansion);
2. **SPEX_BUILD_DIR** - path to the directory, where you are going to build server.
Note that the *SPEX_SOURCE_DIR* and *SPEX_BUILD_DIR* shouldn't be the same directories!

For example, you can initialize them as follow:
```powershell
$SPEX_SOURCE_DIR="$HOME\Projects\space-expansion-server"
$SPEX_BUILD_DIR="$HOME\Projects\space-expansion-server-build"
```

Clone the server's sources:
```powershell
git clone https://github.com/ziminas1990/space-expansion.git $SPEX_SOURCE_DIR
```

Now, to build server run the following commands:
```
cd $SPEX_BUILD_DIR
conan install $SPEX_SOURCE_DIR/conanfile.txt --build=missing
cmake $SPEX_SOURCE_DIR
cmake --build .
```

# Troubleshooting
## Conan
If you got some problem with Conan and want to retry conan build, first you should do is **remove conan cache**. Normally, conan cache is a **$HOME/.conan** directory. If it is exist, just run
```powershell
Remove-Item –path $HOME/.conan –recurse
```

You can also check, that conan found appropriate compiler. After you removed conan cache, you can run conan again. If conan managed to find windows compiler, it will output the following log:
```
Auto detecting your dev setup to initialize the default profile (C:\Users\zimin\.conan\profiles\default)
Found Visual Studio 16
Default settings
        os=Windows
        os_build=Windows
        arch=x86_64
        arch_build=x86_64
        compiler=Visual Studio
        compiler.version=16
        build_type=Release
*** You can change them in C:\Users\zimin\.conan\profiles\default ***
*** Or override with -s compiler='other' -s ...s***
```

When you run cmake configuration for the first time, it should also output the similar log:
```powershell
-- Building for: Visual Studio 16 2019
-- Selecting Windows SDK version 10.0.18362.0 to target Windows 10.0.18363.
-- The C compiler identification is MSVC 19.25.28610.4
-- The CXX compiler identification is MSVC 19.25.28610.4
```

**Make sure** that both cmake and conan use the same compiler!