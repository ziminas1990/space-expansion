cmake_minimum_required(VERSION 3.2)

set(PROJECT_NAME space-expansion-server)

option(autotests-mode "Build autotest binary" OFF)
option(build-debug    "Bulding debug version" OFF)
option(build-32bit    "Bulding 32bit binary"  OFF)
option(spinlocks-only "Using spinlocks instead of mutex" OFF)

project(${PROJECT_NAME} VERSION 0.1.0)

if (build-debug)
  set(CMAKE_BUILD_TYPE Debug)
endif ()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

#=========================================================================================
# Generating protobuf sources:
# Я так и не понял, каким образом нужно генерить исходники protobuf, поэтому делаем
# такой хак: достаём путь к директории с компилятором и генерим код так же
# как и при ручной сборке
set(PROTOBUF_COMPILER ${CONAN_BIN_DIRS_PROTOC_INSTALLER}/protoc)
set(PROTOBUF_FILE   Protocol.proto)
set(PROTOBUF_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Protocol.pb.cc"
                    "${CMAKE_CURRENT_BINARY_DIR}/Protocol.pb.h")
add_custom_command(
  OUTPUT ${PROTOBUF_OUTPUT}
  COMMAND ${PROTOBUF_COMPILER} ${PROTOBUF_FILE} --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  VERBATIM USES_TERMINAL)
add_custom_target(protobuf-generated DEPENDS ${PROTOBUF_OUTPUT})

#=========================================================================================
# Creating Protocol libarary target:
add_library(Protocol ${PROTOBUF_OUTPUT})
add_dependencies(Protocol protobuf-generated)
target_include_directories(Protocol PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
)
set_target_properties(Protocol PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  # Protobufs spawns a lot of warnings (unused variable) in it's generated code,
  # so I won't pass -Wpedantic or -Wall or -Wextra or etc to this target
)
if (build-32bit)
  set_property(TARGET Protocol APPEND_STRING PROPERTY COMPILE_FLAGS " -m32")
endif (build-32bit)

target_link_libraries(Protocol CONAN_PKG::protobuf)

#=========================================================================================
# Creating main target:
file(GLOB_RECURSE ALL_SOURCES_FILES "*.h" "*.cpp")

add_executable(${PROJECT_NAME} ${ALL_SOURCES_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)
set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  COMPILE_OPTIONS "-Wpedantic;-Wall;-Wextra;-Werror=return-type"
)

if (build-32bit)
  set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS " -m32")
  set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS   " -m32")
endif (build-32bit)

if (autotests-mode)
  target_compile_definitions(${PROJECT_NAME} PRIVATE AUTOTESTS_MODE)
endif ()
if (build-debug)
  target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_MODE)
endif()
if (spinlocks-only)
  target_compile_definitions(${PROJECT_NAME} PRIVATE SPINLOCKS_ONLY_MODE)
endif()

target_link_libraries(${PROJECT_NAME} Protocol ${CONAN_TARGETS})

#=========================================================================================
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT ziminas1990@gmail.com)
include(CPack)
