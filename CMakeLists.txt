cmake_policy(SET CMP0091 NEW)
project(gamepad)
cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(GAMEPAD_BUILD_EXAMPLE "Build gamepad example." OFF)
option(BUILD_SHARED_LIBS     "Build gamepad as a shared library" OFF)

if(APPLE)
  enable_language(OBJCXX)
endif()

set(LIBRARY_NAME gamepad)

if(APPLE)
  set(GAMEPAD_SOURCES
    src/gamepad.mm
  )
else()
  set(GAMEPAD_SOURCES
    src/gamepad.cpp
  )
endif()

set(GAMEPAD_HEADERS
  include/gamepad/gamepad.h
)

set(GAMEPAD_PRIVATE_HEADERS
  src/gamepad_internal.h
)

add_library(${LIBRARY_NAME}
  ${GAMEPAD_SOURCES}
  ${GAMEPAD_HEADERS}
  ${GAMEPAD_PRIVATE_HEADERS}
)

set_target_properties(${LIBRARY_NAME} PROPERTIES
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
  POSITION_INDEPENDENT_CODE ON
  C_VISIBILITY_PRESET hidden
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)
 
if(APPLE)
  target_link_libraries(${LIBRARY_NAME}
    PUBLIC
    "-framework Foundation"
    "-framework IOKit"
  )
else()
  target_link_libraries(${LIBRARY_NAME}
    PUBLIC
    $<$<BOOL:${WIN32}>:setupapi>
  )
endif()

target_include_directories(${LIBRARY_NAME}
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

##################
## Examples
if(${GAMEPAD_BUILD_EXAMPLE})

add_executable(${LIBRARY_NAME}_example
  example/main.cpp
  example/conio.cpp
)

set_target_properties(${LIBRARY_NAME}_example PROPERTIES
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
  POSITION_INDEPENDENT_CODE ON
  C_VISIBILITY_PRESET hidden
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)

target_link_libraries(${LIBRARY_NAME}_example
  PRIVATE
  ${LIBRARY_NAME}
)

target_include_directories(${LIBRARY_NAME}_example
  PRIVATE
  include/
)

endif()

##################
## Install rules

add_library(Gamepad::Gamepad ALIAS gamepad)
set_target_properties(gamepad PROPERTIES EXPORT_NAME Gamepad)

##################
## Install rules
install(TARGETS gamepad EXPORT GamepadTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install(FILES ${GAMEPAD_HEADERS} DESTINATION include/gamepad)

# Export targets
install(
  EXPORT GamepadTargets
  FILE GamepadConfig.cmake
  NAMESPACE Gamepad::
  DESTINATION lib/cmake/gamepad
)
