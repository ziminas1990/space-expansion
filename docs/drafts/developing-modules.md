# Developing modules

Steps:
1. Implement module as `BaseModule` and `utils::GlobalObject` inheriter
2. Implement Module's manager. Consider using `DECLARE_DEFAULT_MODULE_MANAGER` macro (see Modules/managers.h)
3. Add corresponding forward declaration to `Modules/Fwd.h`
4. Add module's include to `Modules/All.h`
5. Add module's manager to SystemManager conveyor
6. Add corresponding check to `GlobalContainerUtils::checkAllContainersAreEmpty()`
