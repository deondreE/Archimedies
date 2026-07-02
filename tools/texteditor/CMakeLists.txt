
find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)


add_executable(rune Main.cpp String8.hpp Apptypes.hpp String8.cpp Arena.cpp Piecetable.hpp Piecetable.cpp)

target_link_libraries(rune
  PRIVATE
  SDL3::SDL3
  SDL3_ttf::SDL3_ttf)

target_include_directories(rune
  PUBLIC
  /opt/homebrew/include/SDL3
  /opt/homebrew/include/SDL3_ttf
  .
)
