
find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)

add_executable(node_editor main.cpp NodeTemplates.cpp Helpers.cpp Render.cpp)

target_link_libraries(node_editor
  PRIVATE
  SDL3::SDL3
  SDL3_ttf::SDL3_ttf)

target_include_directories(node_editor
  PUBLIC
  /opt/homebrew/include/SDL3
  /opt/homebrew/include/SDL3_ttf
  .
)
