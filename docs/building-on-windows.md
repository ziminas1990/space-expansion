# Building on Windows

Windows is not a primary target platform for this project, but the server can
be built on Windows 10 or 11 with MSVC.

## Preparing system

Install the following tools:

1. [Build Tools for Visual Studio 2022](https://visualstudio.microsoft.com/downloads/)
   with the **Desktop development with C++** workload;
2. [CMake](https://cmake.org/download/) and add it to `PATH`;
3. [Python](https://www.python.org/downloads/) and add it to `PATH`;
4. [Git](https://git-scm.com/);
5. [Ninja](https://ninja-build.org/) and add it to `PATH` if you intend to
   make a development build.

Install Conan:

```powershell
pip install conan
```

Make sure that the tools are available:

```powershell
cmake --version
git --version
python --version
conan --version
```

## Preparing Conan

Create a default Conan profile:

```powershell
conan profile detect
```

The detected profile should use MSVC, for example:

```text
compiler=msvc
compiler.version=194
```

Make sure that CMake and Conan use compatible compilers.

## Building server

The commands below use the following PowerShell variables:

```powershell
$SPEX_SOURCE_DIR="$HOME\Projects\space-expansion"
$SPEX_BUILD_DIR="$HOME\Projects\space-expansion-build"
$SPEX_VENV_DIR="$HOME\Projects\space-expansion-venv"
```

The source and build directories must be different. Clone the project and
create the build directory:

```powershell
git clone git@github.com:ziminas1990/space-expansion.git $SPEX_SOURCE_DIR
New-Item -ItemType Directory -Force $SPEX_BUILD_DIR
```

## Release build

The default Visual Studio generator is sufficient for a release build:

```powershell
# Building dependencies
conan install "$SPEX_SOURCE_DIR\server\conanfile.txt" --output-folder=$SPEX_BUILD_DIR --build=missing

# Building server
cmake -S "$SPEX_SOURCE_DIR\server" -B $SPEX_BUILD_DIR --preset conan-default
cmake --build $SPEX_BUILD_DIR --config Release --parallel
```

The executable will be placed at:

```powershell
& "$SPEX_BUILD_DIR\Release\space-expansion-server.exe"
```

## Development build

A development build includes debug symbols, autotests and a
`compile_commands.json` file for Clangd.

CMake's Visual Studio generator does not support
`CMAKE_EXPORT_COMPILE_COMMANDS`. Therefore, the development build uses the
Ninja generator with the MSVC compiler. Run these commands from
**Developer PowerShell for VS 2022**, so that `cl.exe` is available.

Use an empty build directory if it was previously configured with the Visual
Studio generator.

```powershell
# Building dependencies and generating a Ninja-based Conan preset
conan install "$SPEX_SOURCE_DIR\server\conanfile.txt" --output-folder=$SPEX_BUILD_DIR --build=missing -s build_type=Debug -c tools.cmake.cmaketoolchain:generator=Ninja

# Configuring and building server and autotests
cmake -S "$SPEX_SOURCE_DIR\server" -B $SPEX_BUILD_DIR -Dbuild-debug=ON -Dwith-autotests=ON --preset conan-debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build $SPEX_BUILD_DIR --parallel
```

Clangd must be pointed to the generated compilation database. One option is to
create a symbolic link in the source directory:

```powershell
New-Item -ItemType SymbolicLink `
  -Path "$SPEX_SOURCE_DIR\compile_commands.json" `
  -Target "$SPEX_BUILD_DIR\compile_commands.json"
```

Creating symbolic links requires either Windows Developer Mode or an elevated
PowerShell session.

To run autotests:

```powershell
& "$SPEX_BUILD_DIR\autotests.exe"
```

To run the server, make sure that `space-expansion.cfg` exists in the working
directory, then run:

```powershell
& "$SPEX_BUILD_DIR\space-expansion-server.exe"
```

## Run integration tests

Create and activate a Python virtual environment:

```powershell
python -m venv $SPEX_VENV_DIR
& "$SPEX_VENV_DIR\Scripts\Activate.ps1"
pip install pyyaml protobuf typing-extensions
```

Set up the environment and run the tests. The path below assumes a release
build made with the Visual Studio generator:

```powershell
$env:PYTHONPATH="$SPEX_SOURCE_DIR\python-sdk"
$env:SPEX_SERVER_BINARY="$SPEX_BUILD_DIR\Release\space-expansion-server.exe"
Set-Location "$SPEX_SOURCE_DIR\tests"
python -m unittest discover
```
