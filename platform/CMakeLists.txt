set(PLATFORM_SOURCES
    src/Platform.cpp
)

if(APPLE)
    list(APPEND PLATFORM_SOURCES src/Application.mm)
    
    set_source_files_properties(
        src/Application.mm
        PROPERTIES LANGUAGE OBJCXX
    )
else()
    list(APPEND PLATFORM_SOURCES src/Application.cpp)
endif()
add_library(archimedes_platform STATIC ${PLATFORM_SOURCES})

if(APPLE)
    target_compile_definitions(archimedes_platform PUBLIC PLATFORM_MACOS)
elseif(WIN32)
    target_compile_definitions(archimedes_platform PUBLIC PLATFORM_WINDOWS)
elseif(UNIX)
    target_compile_definitions(archimedes_platform PUBLIC PLATFORM_LINUX)
endif()

target_include_directories(archimedes_platform PUBLIC 
    include 
    ../engine/kernel/include 
    ../engine/renderer/include
)

target_link_libraries(archimedes_platform
    PUBLIC
    archimedes_core
    archimedes_renderer
    tinygltf
)

if(APPLE)
    target_link_libraries(archimedes_platform
        PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework QuartzCore"
        "-framework Metal"
        "-framework MetalKit"
        "-framework GameController"
    )
endif()