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

find_package(ZLIB REQUIRED)
find_package(protobuf REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(GTest REQUIRED)
find_package(Boost REQUIRED)

set(ALL_DEPENDENCIES ZLIB::ZLIB boost::boost protobuf::protobuf yaml-cpp gtest::gtest)

#=========================================================================================
# Creating Protocol libarary target:

set(PROTOBUF_FILES "CommonTypes.proto" "Protocol.proto" "Privileged.proto")
protobuf_generate_cpp(PROTOBUF_CPP_FILES PROTOBUF_H_FILES ${PROTOBUF_FILES})

add_library(Protocol ${PROTOBUF_CPP_FILES} ${PROTOBUF_H_FILES})
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

target_link_libraries(Protocol protobuf::protobuf)

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

file(GLOB_RECURSE ALL_HEADER_FILES "*.h")

add_executable(${PROJECT_NAME} ${ALL_SOURCES_FILES} ${ALL_HEADER_FILES})
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
  #NOTE: 'WIN32_LEAN_AND_MEAN' is required, otherwise boost.asio will conflict with windows.h
  set_target_properties(${PROJECT_NAME} PROPERTIES
    COMPILE_OPTIONS "-D_USE_MATH_DEFINES;-D_WIN32_WINNT=0x0601;-DNOMINMAX;-DWIN32_LEAN_AND_MEAN"
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

target_link_libraries(${PROJECT_NAME} Protocol ${ALL_DEPENDENCIES})

#=========================================================================================
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT ziminas1990@gmail.com)
include(CPack)
