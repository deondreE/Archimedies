#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <map>
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


/// This refers to a body node, the nodes inside of that body
template <typename T>
struct BodyNodeInner {
  int parent_id;
  std::vector<T> nodes;
};

/** Default node type */
struct Node {
  int id;
  std::string name;
  std::string internalFunction;

  // @TODO: Figure out if I actually use these
  std::optional<std::vector<Pin>> inputs;
  std::optional<std::vector<Pin>> outputs;
  int input_count = 1;
  int output_count = 1;

  Color color;
  Rect UI_bounds;
  std::string custom_value = "0.0f";
  bool needs_body = false;
  std::optional<BodyNodeInner<Node>> inner_node = std::nullopt;
};

struct NodePreset {
  std::string type_name;
  std::string default_label;
  SDL_Color color;
  int inputs;
  int outputs;
  std::string code_template;
  bool needs_body;
};

struct AppContext {
  TTF_Font *font = nullptr;
  float fontSize = 16.0f;
  std::string fontPath =
      "/Users/deondreenglish/Library/Fonts/MapleMono-Regular.ttf";
  std::map<char, Glyph> glyphCache;
  float charW = 0.0f, charH = 0.0f;
};

struct MouseState {
  Vector2 pos;
};

struct DraggingState {
  bool is_dragging_connection = false;
  bool is_dragging_node = false;
  bool is_resizing_node = false;
  Vector2 start_pos;
  int active_node_id = -1;
  int active_pin_id = -1;
  Vector2 drag_offset; // Stores mouse distance from top-left of node
};

enum class NodeRegion { BODY, BOTTOM_LEFT, BOTTOM_RIGHT, TOP_LEFT, TOP_RIGHT };

extern std::vector<NodePreset> g_node_presets;
extern bool is_point_in_circle(float px, float py, float cx, float cy,
                               float radius);
extern bool is_point_in_rect(float px, float py, SDL_FRect rect);
extern Node *FindNodeAtPoint(std::vector<Node> &nodes, Vector2 pos);
extern void BuildGlyphCache(AppContext &app, SDL_Renderer *renderer);
extern NodeRegion GetNodeRegion(Vector2 mousePos, const Rect &bounds,
                                float handleSize);
extern Vector2 get_pin_pos(const Node &node, bool is_output, int pin_idx,
                           bool needs_body = false);

extern void draw_bezier(SDL_Renderer *renderer, Vector2 start, Vector2 end,
                             float thickness = 2.0f);
extern void DrawText(AppContext &app, const std::string &text, float x, float y,
                SDL_Color color, SDL_Renderer *renderer);
extern void draw_circle(SDL_Renderer *renderer, float cx, float cy,
                        float radius, SDL_FColor color);
