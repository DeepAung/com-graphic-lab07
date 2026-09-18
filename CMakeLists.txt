cmake_minimum_required(VERSION 3.24)
project(lab07 LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR
    "in-source builds overwrite this project's Makefile. Use: cmake -S . -B build")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Vulkan
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

# GLM ships with the Vulkan SDK, but not always on the default include path
find_path(GLM_INCLUDE_DIR glm/glm.hpp
  HINTS $ENV{VULKAN_SDK}/include ${Vulkan_INCLUDE_DIR}
  PATHS /usr/local/include /opt/homebrew/include /usr/include
)

if(NOT GLM_INCLUDE_DIR)
  message(FATAL_ERROR "glm/glm.hpp not found. It comes with the Vulkan SDK: set VULKAN_SDK "
                      "or source the SDK's setup-env.sh. On Linux with apt, install libglm-dev")
endif()

add_library(glm INTERFACE)
target_include_directories(glm SYSTEM INTERFACE ${GLM_INCLUDE_DIR})

# stb is two headers, fetched once, used only by src/utils/image.cpp
set(STB_COMMIT f0569113c93ad095470c54bf34a17b36646bbbb5)
set(STB_DIR ${CMAKE_SOURCE_DIR}/external/stb)

foreach(HEADER stb_image.h stb_image_write.h)
  if(NOT EXISTS ${STB_DIR}/${HEADER})
    message(STATUS "fetching ${HEADER}")
    file(DOWNLOAD
      https://raw.githubusercontent.com/nothings/stb/${STB_COMMIT}/${HEADER}
      ${STB_DIR}/${HEADER}
      STATUS STB_STATUS
      TLS_VERIFY ON
    )
    list(GET STB_STATUS 0 STB_CODE)
    if(NOT STB_CODE EQUAL 0)
      file(REMOVE ${STB_DIR}/${HEADER})
      message(FATAL_ERROR "could not download ${HEADER}. Run external/fetch.sh, "
                          "or copy the two stb headers into external/stb by hand")
    endif()
  endif()
endforeach()

file(SHA256 ${STB_DIR}/stb_image.h STB_IMAGE_HASH)
if(NOT STB_IMAGE_HASH STREQUAL
   "594c2fe35d49488b4382dbfaec8f98366defca819d916ac95becf3e75f4200b3")
  message(FATAL_ERROR "external/stb/stb_image.h does not match the pinned commit")
endif()

add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE ${STB_DIR})

# ── Where the shaders come from ───────────────────────────────────────────────
# This week you edit shaders, not C++. Point STUDENT_SHADERS at another directory
# and any file with the same name there wins over shaders/, for glslc and for the
# hot reload. That is how the solution is built:
#   cmake -S . -B build -DSTUDENT_SHADERS=../lab07-solution/shaders
# A relative path is read from this file's directory. STRING, not PATH: CMake would
# rewrite a PATH against the shell's working directory instead. This is also why the
# shaders say #include <noise.glsl> and not "noise.glsl": angle brackets search the
# include path, where the student directory comes first, instead of looking next to
# the including file.
set(STUDENT_SHADERS "" CACHE STRING "directory whose shaders override shaders/")

set(SHADER_DIRS "${CMAKE_SOURCE_DIR}/shaders")
if(STUDENT_SHADERS)
  get_filename_component(STUDENT_DIR "${STUDENT_SHADERS}" ABSOLUTE BASE_DIR ${CMAKE_SOURCE_DIR})
  if(NOT IS_DIRECTORY "${STUDENT_DIR}")
    message(FATAL_ERROR "STUDENT_SHADERS is not a directory: ${STUDENT_DIR}")
  endif()
  set(SHADER_DIRS "${STUDENT_DIR}" "${CMAKE_SOURCE_DIR}/shaders")
  message(STATUS "student shaders: ${STUDENT_DIR}")
endif()

# ── Core library ──────────────────────────────────────────────────────────────
add_library(lab07core STATIC
  src/core/Device.cpp
  src/core/Buffer.cpp
  src/core/Target.cpp
  src/core/Pipeline.cpp
  src/core/Shader.cpp
  src/core/Frame.cpp
  src/core/Timer.cpp
  src/core/Window.cpp
  src/utils/Args.cpp
  src/utils/utils.cpp
  src/utils/image.cpp
)

target_include_directories(lab07core PUBLIC
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(lab07core PUBLIC
  Vulkan::Vulkan
  glfw
  glm
  stb
)

target_compile_options(lab07core PUBLIC -Wall -Wextra)

# ── Shader compilation ────────────────────────────────────────────────────────
# glslc ships with the Vulkan SDK; glslangValidator is the fallback name.
find_program(GLSLC NAMES glslc glslangValidator)

if(NOT GLSLC)
  message(FATAL_ERROR "no GLSL compiler found. glslc comes with the Vulkan SDK; "
                      "on Linux, apt install glslang-tools")
endif()

# One string, the directories separated by ';', student directory first. The separator
# has to be escaped and the whole argument quoted, or CMake reads the ';' as a list
# separator and splits one -D flag into two.
string(JOIN ";" SHADER_SRC_DIRS ${SHADER_DIRS})
string(REPLACE ";" "\;" SHADER_SRC_DIRS "${SHADER_SRC_DIRS}")

target_compile_definitions(lab07core PUBLIC
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_RADIANS
  SHADER_DIR="${CMAKE_BINARY_DIR}/shaders"
  GLSLC="${GLSLC}"
  REFERENCE_DIR="${CMAKE_SOURCE_DIR}/reference"
  REPORT_DIR="${CMAKE_SOURCE_DIR}/report"
  "SHADER_SRC_DIRS=\"${SHADER_SRC_DIRS}\""
)

# ── One executable per part ───────────────────────────────────────────────────
set(PARTS info circle terrain sdf heightmap toy)

foreach(PART ${PARTS})
  add_executable(${PART} src/${PART}.cpp)
  target_link_libraries(${PART} PRIVATE lab07core)
endforeach()

# .glsl files are includes only: never compiled, but every shader depends on them
set(GLSL_INCLUDES "")
foreach(DIR ${SHADER_DIRS})
  file(GLOB DIR_INCLUDES "${DIR}/*.glsl")
  list(APPEND GLSL_INCLUDES ${DIR_INCLUDES})
  list(APPEND GLSLC_INCLUDE_FLAGS "-I${DIR}")
endforeach()

file(GLOB GLSL_FILES
  "${CMAKE_SOURCE_DIR}/shaders/*.vert"
  "${CMAKE_SOURCE_DIR}/shaders/*.frag"
)

set(SPIRV_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY ${SPIRV_OUTPUT_DIR})

if(GLSLC MATCHES "glslangValidator")
  set(GLSLC_ARGS -V)
endif()

foreach(GLSL ${GLSL_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SOURCE ${GLSL})
  if(STUDENT_DIR AND EXISTS "${STUDENT_DIR}/${FILE_NAME}")
    set(SOURCE "${STUDENT_DIR}/${FILE_NAME}")
  endif()
  set(SPIRV "${SPIRV_OUTPUT_DIR}/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSLC} ${GLSLC_ARGS} ${GLSLC_INCLUDE_FLAGS} ${SOURCE} -o ${SPIRV}
    DEPENDS ${SOURCE} ${GLSL_INCLUDES}
  )
  list(APPEND SPIRV_FILES ${SPIRV})
endforeach()

add_custom_target(shaders DEPENDS ${SPIRV_FILES})
add_dependencies(lab07core shaders)
