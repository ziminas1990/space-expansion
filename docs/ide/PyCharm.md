# Configuring PyCharm on Linux
Last tested on Ubuntu 23.04

### Open project
Just open an existing project by specifying `$SPEX_ROOT_DIR` as project's root directory.
Go to `Setting -> Project -> Project Structure` and add the following content root:
- $SPEX_SOURCE_DIR/python-sdk
- $SPEX_SOURCE_DIR/tests

### Configure interpreter
In `Settings -> Projects -> Python Interpreter` specify an `Existing environment` by providing path to `$SPEX_VENV_DIR/bin/python` interpreter. This interpreter should now be specified in all run configurations.
Add the following directories to the interpreter paths:
* $SPEX_SOURCE_DIR/python-sdk

### Running functional tests
Go to `Run -> Edit Configurations...` and create a new `Python Tests -> Unittests` configuration.

Specify `$SPEX_SOURCE_DIR/tests` as a `script path`.

Add the following environment variables to configuration:
1. PYTHONPATH=$SPEX_SOURCE_DIR/python-sdk
2. SPEX_SERVER_BINARY=$SPEX_BUILD_DIR/space-expansion-server

