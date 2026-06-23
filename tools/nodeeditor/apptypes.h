#pragma once

#include <SDL.h>
#include <optional>
#include <string>
#include <vector>

struct Color {
  float r, g, b, a;

  Color() {}
  Color(SDL_Color color)
      : r{(float)color.r}, g{(float)color.g}, b{(float)color.b},
        a{(float)color.a} {}
  Color(float r, float g, float b, float a) : r{r}, g{g}, b{b}, a{a} {}
};

struct Rect {
  float x, y, w, h;
};

struct Vector3 {
  float x, y, z;
};

struct Vector2 {
  float x, y;
};

struct Connection {
  int from_node_id;
  int from_pin_idx;
  int to_node_id;
  int to_pin_idx;
};

struct Glyph {
  SDL_Texture *texture;
  int w, h;
};

enum class PinDirection { INPUT, OUTPUT };

enum class DataType { FLOW, FLOAT, INT, VEC3, STRING };
struct Pin {
  int id;
  int nodeId;
  std::string name;
  PinDirection direction;
  DataType dataType;
};

struct Link {
  int id;
  int startPinId;
  int endPinId;
};

/** Default node type */
struct Node {
  int id;
  std::string name;
  std::string internalFunction;

  std::optional<std::vector<Pin>> inputs;
  std::optional<std::vector<Pin>> outputs;
  int input_count = 1;
  int output_count = 1;

  Color color;
  Rect UI_bounds;
  std::string custom_value = "0.0f";
};

struct NodePreset {
    std::string type_name;
    std::string default_label;
    SDL_Color color;
    int inputs;
    int outputs;
    std::string code_template;
};

extern std::vector<NodePreset> g_node_presets;