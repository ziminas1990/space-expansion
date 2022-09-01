# Deploy web-client on Windows 10

## Check if everything ready to start
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
$WEB_CLIENT_VENV="$PROJECTS_DIR\web-client-venv"
```

## Checkout space-expansion repo
To checkout space-expansion repo and switch to stable branch:
```powershell
git clone https://github.com/ziminas1990/space-expansion.git $MONOREPO
cd $MONOREPO
git checkout stable
```

## Setup python virtual enviroment
```powershell
py -m venv create $WEB_CLIENT_VENV
```
Before you can activate virtual environment in your powershell session, run the followin command:
```powershell
Set-ExecutionPolicy -ExecutionPolicy Unrestricted -Scope CurrentUser
```
Now activate your virtual enviroment:
```
cd $WEB_CLIENT_VENV
Scripts\Activate
```
**Selfcheck:** make sure that **pip** command calls a `pip.exe` from your virtual enviroment directory:
```
gcm pip
```
You should get a table with `pip.exe`, placed in you $WEB_CLIENT_VENV subfolder.

Install the following required packages:
```
pip install quart protobuf==3.9.1 flask-login
```

## Run the server
### Prepare your powershell session
If you have just run a new PowerShell session, don't forget to [set variables](#configure-your-pathes).

```powershell
# Activate the enviroment, if it is not activated yet
cd $WEB_CLIENT_VENV
Scripts\Activate
# Configure PYTHONPATH
$env:PYTHONPATH="$MONOREPO\python-sdk"

cd $MONOREPO\web-client
$env:QUART_APP="space-assistant"
```

### Run the quart
```powershell
quart run --host 0.0.0.0
```

If everything is right, you will see the following line:
```
INFO:quart.serving:Running on http://0.0.0.0:5000 (CTRL + C to quit)
```

It means that quart is receiving incomming HTTP reuest at 5000 port.

TO connect to the server open [localhost:5000](http://localhost:5000) in your browser.
