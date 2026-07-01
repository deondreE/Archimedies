
set(RENDERER_SOURCES
    renderer/src/Renderer.cpp
)

add_library(archimedes_core STATIC
  kernel/src/kernel.cpp
  kernel/src/gltfsource.cpp
  kernel/src/Serializer.cpp
)

# add_library(tinygltf INTERFACE)
target_include_directories(archimedes_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/tinygltf
    ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/stb
)
add_library(archimedes_renderer STATIC
  ${RENDERER_SOURCES}
)

if (APPLE)
  target_sources(archimedes_renderer PRIVATE renderer/src/MetalRenderer.mm renderer/src/MetalViewport.mm)


  target_include_directories(archimedes_renderer
        PRIVATE /opt/homebrew/opt/spirv-cross/include/spirv_cross
        PRIVATE /opt/homebrew/include/freetype2
    )


  # @TODO: pkg-config
  target_link_libraries(archimedes_renderer
        PRIVATE
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore"
        /opt/homebrew/opt/spirv-cross/lib/libspirv-cross-msl.a
        /opt/homebrew/opt/spirv-cross/lib/libspirv-cross-core.a
        /opt/homebrew/opt/spirv-cross/lib/libspirv-cross-c.a
        /opt/homebrew/opt/spirv-cross/lib/libspirv-cross-cpp.a
        /opt/homebrew/opt/spirv-cross/lib/libspirv-cross-glsl.a
        /opt/homebrew/opt/spirv-cross/lib/libspirv-cross-hlsl.a
        /opt/homebrew/opt/spirv-cross/lib/libspirv-cross-reflect.a
        /opt/homebrew/opt/spirv-cross/lib/libspirv-cross-util.a
        /opt/homebrew/lib/libfreetype.a
        /opt/homebrew/lib/libpng16.a
        -lz
        -lbz2
    )

endif()


target_include_directories(archimedes_core
  PUBLIC kernel/include
)

# target_include_directories(archimedes_core PUBLIC
#     ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/tinygltf
#     ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/stb
# )

target_link_libraries(archimedes_core PRIVATE
  tinygltf
)

target_include_directories(archimedes_renderer
  PUBLIC renderer/include
  PUBLIC kernel/include
)

target_link_libraries(archimedes_renderer
  PRIVATE archimedes_core
)
set(CORE_PCH "${CMAKE_CURRENT_SOURCE_DIR}/pch.h")

target_precompile_headers(archimedes_core PUBLIC
    "$<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:OBJCXX>>:${CORE_PCH}>"
)


target_precompile_headers(archimedes_renderer PRIVATE
    "$<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:OBJCXX>>:${CORE_PCH}>"
)
