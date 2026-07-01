cmake_minimum_required(VERSION 3.25)
project(Archimedies VERSION 1.0 LANGUAGES CXX)

if (APPLE)
  enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(GLSL_VALIDATOR glslangValidator)
if(NOT GLSL_VALIDATOR)
  message(FATAL_ERROR "glslangValidator not found! Please install the Vulkan SDK.")
endif()

set(SHADER_INPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/engine/shaders")
set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")
set(MODEL_DIR "${CMAKE_BINARY_DIR}/../assets/models")

macro(compile_shader SHADER_PATH)
  if(NOT EXISTS ${SHADER_PATH})
    message(FATAL_ERROR "Shader source file NOT FOUND: ${SHADER_PATH}")
  endif()

  get_filename_component(SHADER_NAME ${SHADER_PATH} NAME)
  set(OUTPUT_FILE "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")

  add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
        COMMAND ${GLSL_VALIDATOR} -V ${SHADER_PATH} -o ${OUTPUT_FILE}
        DEPENDS ${SHADER_PATH}
        COMMENT "Compiling shader ${SHADER_NAME} to SPIR-V"
    )

  list(APPEND ALL_SHADER_OUTPUTS ${OUTPUT_FILE})
endmacro()

set(GLSL_SHADERS
    "${SHADER_INPUT_DIR}/test.vert"
    "${SHADER_INPUT_DIR}/test.frag"
)

foreach(SHADER ${GLSL_SHADERS})
  compile_shader(${SHADER})
endforeach()

# 5. Create the Target
add_custom_target(compiled_shaders DEPENDS ${ALL_SHADER_OUTPUTS})

set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF CACHE BOOL "" FORCE)
set(TINYGLTF_HEADER_ONLY OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/tinygltf)

target_sources(tinygltf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tinygltf/tiny_gltf_v3.c)
target_compile_definitions(tinygltf PRIVATE TINYGLTF3_ENABLE_FS)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/vendor/tinygltf/tiny_gltf_v3.c LANGUAGE C)


add_subdirectory(engine)
add_subdirectory(platform)
add_subdirectory(tools/hexeditor)
add_subdirectory(tools/nodeeditor)


add_executable(app main.cpp)
add_dependencies(app compiled_shaders)

if(APPLE)
  set_target_properties(app PROPERTIES LINKER_LANGUAGE CXX)
endif()

target_link_libraries(app
    PRIVATE
    archimedes_platform
    archimedes_renderer
    archimedes_core
    tinygltf
)

target_precompile_headers(app REUSE_FROM archimedes_core)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
  target_compile_options(app PRIVATE
        -Wall -Wextra -Wpedantic -Werror -fcolor-diagnostics
    )
endif()

enable_testing()
add_subdirectory(tests)

target_compile_definitions(archimedes_platform PRIVATE
    SHADER_DIR="${SHADER_OUTPUT_DIR}/"
)
target_compile_definitions(archimedes_platform PRIVATE
    MODEL_DIR="${MODEL_DIR}/"
)
add_compile_definitions(SHADER_DIR="${SHADER_OUTPUT_DIR}/")
