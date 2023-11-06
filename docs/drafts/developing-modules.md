# Developing modules

Steps:
1. Implement module as `BaseModule` and `utils::GlobalObject` inheriter
2. Implement Module's manager. Consider using `DECLARE_DEFAULT_MODULE_MANAGER` macro (see Modules/managers.h)
3. Add corresponding forward declaration to `Modules/Fwd.h`
4. Add module's include to `Modules/All.h`
5. Add module's manager to SystemManager conveyor (see `SystemManager::linkComponents()`)
6. Add corresponding check to `GlobalContainerUtils::checkAllContainersAreEmpty()`

Unit tests steps:
1. Implement module as `ClientBaseModule` inheriter in `Autotests/ClientSDK/Modules`
2. Create a corresponding test fixture as `ModulesTestFixture` inheriter in `Autotests/Modules` (see MessangerTests.cpp)

Extend PythonSDK:
1. Run `regenerate_python_files.sh` in the project's root directory. NOTE: it may require you to install `protoc` compiler. Make sure that it's version is close to one specified in `server/conanfile.txt`
2. Declare interface in `python-sdk/expansion/api/__init__.py`
3. Implement a wrapper in `python-sdk/expansion/interfaces/rpc`
4. Declare wrapper in `python-sdk/expansion/interfaces/rpc/__init__.py`
5. Using rpc wrapper implement new module as `BaseModule` inheriter in `python-sdk/expansion/modules`
6. Declare module in `python-sdk/expansion/modules/__init__.py`
6. Register module in `ModuleType` enum in `python-sdk/expansion/modules/base_module.py`
7. Add module instantiation in `module_factory()` in `python-sdk/expansion/modules/factory.py`

Functional tests (using PythonSDK):
1.