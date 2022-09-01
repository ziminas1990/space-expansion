# Run demo client on Windows 10

## Check your enviroment
This guide assumes, that you already have:
1. a `space-expansion-server` is running and avaliable
2. a [git](https://git-scm.com/) and a [python3](https://www.python.org/downloads/) installed on your PC and avaliable thorugh the powershell.

All commands in this manual should be run in **powershell**. To run powershell just press "WIN+R", type `powershell` and press *enter*.

**Selfcheck:** check that **git** in installed:
```powershell
git --version
```
**Selfcheck:** check that **python3** is installed:
```powershell
py --version
```
**Selfcheck:** check that **pip** is installed:
```powershell
py -m pip --version
```

## Configure your pathes
In your powershell configure some variables for further usage. Please, feel free to use another paths you want.
```powershell
$PROJECTS_DIR="$HOME\Projects"
$MONOREPO="$PROJECTS_DIR\space-expansion"
$DEMO_CLIENT_VENV="$PROJECTS_DIR\demo-client-venv"
```
**Note**: make sure, that `$PROJECTS_DIR` contains NO cyrrilic symbols and/or spaces!

## Checkout demo-client repo
To checkout space-expansion repo and switch to stable branch:
```powershell
git clone https://github.com/ziminas1990/space-expansion.git $MONOREPO
cd $MONOREPO
git checkout stable
```

## Setup python virtual enviroment
```powershell
py -m venv create $DEMO_CLIENT_VENV
```
Before you can activate virtual environment in your powershell session, run the followin command:
```powershell
Set-ExecutionPolicy -ExecutionPolicy Unrestricted -Scope CurrentUser
```
Now activate your virtual enviroment:
```
cd $DEMO_CLIENT_VENV
Scripts\Activate
```
**Selfcheck:** make sure that **pip** command calls a `pip.exe` from your virtual enviroment directory:
```
gcm pip
```
You should get a table with `pip.exe`, placed in you $DEMO_CLIENT_VENV subfolder.

Install the following required packages:
```
pip install protobuf==3.9.1 pyqt5
```

## Run the client
If you have just run a new PowerShell session, don't forget to [set variables](#configure-your-pathes) and activate python venv.
```powershell
# Configure PYTHONPATH
$env:PYTHONPATH="$MONOREPO\python-sdk"

# Run the client
cd $MONOREPO\demo
py ./main.py
```
