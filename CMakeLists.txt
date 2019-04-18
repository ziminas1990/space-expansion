cmake_minimum_required(VERSION 3.2)

set(PROJECT_NAME space-expansion-server)

project(${PROJECT_NAME} VERSION 0.1.0)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

#=========================================================================================
# Generating protobuf sources:
# Я так и не понял, каким образом нужно генерить исходники конан, поэтому делаем
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
# Creating main target:
file(GLOB_RECURSE ALL_SOURCES_FILES "*.h" "*.cpp")

add_executable(${PROJECT_NAME} ${ALL_SOURCES_FILES} ${PROTOBUF_OUTPUT})
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)
set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  COMPILE_OPTIONS "-Wpedantic;-Wall;-Wextra"
)
add_dependencies(${PROJECT_NAME} protobuf-generated)
target_link_libraries(${PROJECT_NAME} ${CONAN_TARGETS})

#=========================================================================================
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT ziminas1990@gmail.com)
include(CPack)
