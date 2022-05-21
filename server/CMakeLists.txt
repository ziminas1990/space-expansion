cmake_minimum_required(VERSION 3.2)

set(PROJECT_NAME space-expansion-server)

option(autotests-mode "Build autotest binary" OFF)
option(build-debug    "Bulding debug version" OFF)
option(build-32bit    "Bulding 32bit binary"  OFF)
option(spinlocks-only "Using spinlocks instead of mutex" OFF)
option(with-asan      "Use address sanitizer" OFF)

project(${PROJECT_NAME} VERSION 0.1.0)

if (WIN32 AND with-asan)
  message(FATAL_ERROR, "Addres sanitizer (with-asan) is not avaliable")
endif ()

if (build-debug)
  set(CMAKE_BUILD_TYPE Debug)
endif ()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

#=========================================================================================
# Generating protobuf sources:
# Have no idea how to run protobuf compiler in a proper way. So, here is a hack: just
# get path to compiler and run the compiler manually.
set(PROTOBUF_COMPILER ${CONAN_BIN_DIRS_PROTOBUF}/protoc)

set(PROTOBUF_FILES   "CommonTypes.proto" "Protocol.proto" "Privileged.proto")
set(PROTOBUF_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/CommonTypes.pb.cc"
                    "${CMAKE_CURRENT_BINARY_DIR}/CommonTypes.pb.h"
                    "${CMAKE_CURRENT_BINARY_DIR}/Protocol.pb.cc"
                    "${CMAKE_CURRENT_BINARY_DIR}/Protocol.pb.h"
                    "${CMAKE_CURRENT_BINARY_DIR}/Privileged.pb.cc"
                    "${CMAKE_CURRENT_BINARY_DIR}/Privileged.pb.h")
add_custom_command(
  OUTPUT ${PROTOBUF_OUTPUT}
  COMMAND ${PROTOBUF_COMPILER} ${PROTOBUF_FILES} --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
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
file(GLOB_RECURSE ALL_SOURCES_FILES 
     "${CMAKE_SOURCE_DIR}/AdministratorPanel/*.cpp"
     "${CMAKE_SOURCE_DIR}/Arbitrators/*.cpp"
     "${CMAKE_SOURCE_DIR}/Autotests/*.cpp"
     "${CMAKE_SOURCE_DIR}/Blueprints/*.cpp"
     "${CMAKE_SOURCE_DIR}/ConfigDI/*.cpp"
     "${CMAKE_SOURCE_DIR}/Conveyor/*.cpp"
     "${CMAKE_SOURCE_DIR}/ConveyorTools/*.cpp"
     "${CMAKE_SOURCE_DIR}/Geometry/*.cpp"
     "${CMAKE_SOURCE_DIR}/Modules/*.cpp"
     "${CMAKE_SOURCE_DIR}/Network/*.cpp"
     "${CMAKE_SOURCE_DIR}/Newton/*.cpp"
     "${CMAKE_SOURCE_DIR}/Ships/*.cpp"
     "${CMAKE_SOURCE_DIR}/Utils/*.cpp"
     "${CMAKE_SOURCE_DIR}/World/*.cpp")

list(APPEND ALL_SOURCES_FILES 
     ${CMAKE_SOURCE_DIR}/main.cpp
     ${CMAKE_SOURCE_DIR}/SystemManager.cpp)

add_executable(${PROJECT_NAME} ${ALL_SOURCES_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)
set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
)

if (UNIX)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    COMPILE_OPTIONS "-Wpedantic;-Wall;-Wextra;-Werror=return-type"
  )
elseif (WIN32)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    COMPILE_OPTIONS "-D_USE_MATH_DEFINES;-D_WIN32_WINNT=0x0601;-DNOMINMAX"
  )
endif ()

if (build-32bit)
  set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS " -m32")
  set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS   " -m32")
endif (build-32bit)

if (with-asan AND UNIX)
  set_property(TARGET ${PROJECT_NAME}
               APPEND_STRING PROPERTY COMPILE_FLAGS
               " -fsanitize=address -fno-omit-frame-pointer")
  set_property(TARGET ${PROJECT_NAME}
               APPEND_STRING PROPERTY LINK_FLAGS " -fsanitize=address")
endif ()

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
