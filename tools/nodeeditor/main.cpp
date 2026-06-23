#include "GraphCompiler.h"
#include "NodeSerializer.h"
#include "apptypes.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cstdio>
#include <fstream>
#include <map>
#include <optional>
#include <vector>

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 920;

struct MouseState {
  Vector2 pos;
};

std::vector<NodePreset> g_node_presets = {
    {"Math: Add", "+", {70, 130, 180, 255}, 2, 1, "float {out} = {0} + {1};"},
    {"Array",
     "Array",
     {100, 100, 100, 255},
     1,
     1,
     "std::vector<float> {out} = {{0}};"},
    {"Array Get",
     "GET",
     {100, 100, 100, 255},
     2,
     1,
     "float {out} = {0}[static_cast<int>({1})];"},
    {"Array push",
     "ARRAY PUSH",
     {100, 100, 100, 255},
     2,
     0,
     "{0}.push_back({1});"},
    {"Logic: If", "IF", {220, 20, 60, 255}, 2, 1, "if ({0} {val} {1}) {}"},
    {"Variable", "VAR", {46, 139, 87, 255}, 0, 1, "float {out} = {val};"},
    {"Consoe Log",
     "LOG",
     {200, 20, 60, 255},
     1,
     0,
     "std::cout << {0} << std::endl;"},
    {"For Loop",
     "FOR",
     {20, 20, 122, 255},
     1,
     0,
     "for (int i_{id} = 0; i_{id} < static_cast<int>({0}); ++i_{id}) {}"},
    {"Engine Call", "ENGINE_VAR_NAME", {225, 225, 225, 255}, 2, 1, "{val}();"},
    {"Comment", "COMMENT", {255, 255, 255, 255}, 0, 0, "// {val}"},
    {"Function", "FUNC", {0, 255, 0, 255}, 0, 0, "auto {val} = [&](){};\n\t{val}();"}};
// @TODO: For Loops don't generate properly.
// @TODO: Functions need bodies, -> Group of nodes: see excalidraw
// @TODO: Make body node.
// @TODO: Engine Calls need to be externed at the top of the file.
// @TODO: Script runner.
// @TODO: Nodes should scale to fit their content.

struct DraggingState {
  bool is_dragging_connection = false;
  bool is_dragging_node = false;
  Vector2 start_pos;
  int active_node_id = -1;
  int active_pin_id = -1;
  Vector2 drag_offset; // Stores mouse distance from top-left of node
};

struct AppContext {
  TTF_Font *font = nullptr;
  float fontSize = 16.0f;
  std::string fontPath =
      "/Users/deondreenglish/Library/Fonts/MapleMono-Regular.ttf";
  std::map<char, Glyph> glyphCache;
  float charW = 0.0f, charH = 0.0f;
};

// Iterates through glyph cache to draw strings character by character
void DrawText(AppContext &app, const std::string &text, float x, float y,
              SDL_Color color, SDL_Renderer *renderer) {
  for (char c : text) {
    char lookup = app.glyphCache.contains(c) ? c : '.';
    Glyph &g = app.glyphCache[lookup];
    SDL_SetTextureColorMod(g.texture, color.r, color.g, color.b);
    SDL_FRect dest = {x, y, (float)g.w, (float)g.h};
    SDL_RenderTexture(renderer, g.texture, nullptr, &dest);
    x += app.charW;
  }
}

void ClearGlyphCache(AppContext &app) {
  for (auto &[c, g] : app.glyphCache) {
    SDL_DestroyTexture(g.texture);
  }
  app.glyphCache.clear();
}

// Pre-renders characters into textures for fast UI rendering
void BuildGlyphCache(AppContext &app, SDL_Renderer *renderer) {
  ClearGlyphCache(app);
  std::string charset = "0123456789ABCDEF "
                        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@"
                        "#$%^&*()_+-=[]{};':\",./<>?\\|`~.";
  SDL_Color white = {255, 255, 255, 255};

  for (char c : charset) {
    std::string s(1, c);
    SDL_Surface *surf = TTF_RenderText_Blended(app.font, s.c_str(), 0, white);
    if (surf) {
      app.glyphCache[c] = {SDL_CreateTextureFromSurface(renderer, surf),
                           surf->w, surf->h};
      app.charW = (float)surf->w;
      app.charH = (float)surf->h;
      SDL_DestroySurface(surf);
    }
  }
}

bool is_point_in_circle(float px, float py, float cx, float cy, float radius) {
  float dx = px - cx, dy = py - cy;
  return (dx * dx + dy * dy) <= (radius * radius);
}

bool is_point_in_rect(float px, float py, SDL_FRect rect) {
  return (px >= rect.x && px <= rect.x + rect.w && py >= rect.y &&
          py <= rect.y + rect.h);
}

Vector2 get_pin_pos(const Node &node, bool is_output, int pin_idx) {
  int total_pins = is_output ? node.output_count : node.input_count;
  float x =
      is_output ? (node.UI_bounds.x + node.UI_bounds.w) : node.UI_bounds.x;

  // Distribute pins evenly along the height
  float section_h = node.UI_bounds.h / (float)(total_pins + 1);
  float y = node.UI_bounds.y + (section_h * (pin_idx + 1));

  return {x, y};
}

// Draws a Cubic Bezier curve between ports for professional line look
void draw_bezier(SDL_Renderer *renderer, Vector2 start, Vector2 end) {
  float offset = SDL_fabsf(end.x - start.x) / 2.0f;
  if (offset < 50.0f)
    offset = 50.0f;

  Vector2 cp1 = {start.x + offset, start.y};
  Vector2 cp2 = {end.x - offset, end.y};

  const int steps = 30;
  Vector2 prev = start;
  for (int i = 1; i <= steps; i++) {
    float t = (float)i / (float)steps;
    float invT = 1.0f - t;
    float x = (invT * invT * invT * start.x) + (3 * invT * invT * t * cp1.x) +
              (3 * invT * t * t * cp2.x) + (t * t * t * end.x);
    float y = (invT * invT * invT * start.y) + (3 * invT * invT * t * cp1.y) +
              (3 * invT * t * t * cp2.y) + (t * t * t * end.y);
    SDL_RenderLine(renderer, prev.x, prev.y, x, y);
    prev = {x, y};
  }
}

// Manually draws a filled circle via point-strips
void draw_circle(SDL_Renderer *renderer, float cx, float cy, float radius) {
  for (int w = 0; w < radius * 2; w++) {
    for (int h = 0; h < radius * 2; h++) {
      float dx = radius - w, dy = radius - h;
      if ((dx * dx + dy * dy) <= (radius * radius))
        SDL_RenderPoint(renderer, cx + dx, cy + dy);
    }
  }
}

int g_id_counter = -1;
int g_selected_node_id = -1;
Node CreateNodeFromPreset(const NodePreset &preset, MouseState mState) {
  g_id_counter++;
  return {g_id_counter,
          preset.default_label,
          preset.type_name,
          std::nullopt,
          std::nullopt,
          preset.inputs,
          preset.outputs,
          Color(preset.color),
          {mState.pos.x, mState.pos.y, 150.0f, 100.0f}};
}

Node CreateNode(const std::string &name, MouseState mState) {
  g_id_counter++;
  return {g_id_counter,
          name,
          "Generic",
          std::nullopt,
          std::nullopt,
          1,
          1,
          Color(100, 100, 100, 255),
          {mState.pos.x, mState.pos.y, 150.0f, 100.0f}};
}

void CompactNodeIds(std::vector<Node> &nodes,
                    std::vector<Connection> &connections) {
  if (nodes.empty()) {
    g_id_counter = -1;
    connections.clear();
    return;
  }

  std::map<int, int> id_map;
  for (int i = 0; i < nodes.size(); ++i) {
    id_map[nodes[i].id] = i;
    nodes[i].id = i;
  }

  for (auto it = connections.begin(); it != connections.end();) {
    if (id_map.contains(it->from_node_id) && id_map.contains(it->to_node_id)) {
      it->from_node_id = id_map[it->from_node_id];
      it->to_node_id = id_map[it->to_node_id];
      ++it;
    } else {
      it = connections.erase(it);
    }
  }

  // Set the global counter to the last assigned ID
  g_id_counter = (int)nodes.size() - 1;
}

Node *GetNodeById(std::vector<Node> &nodes, int id) {
  if (id == -1)
    return nullptr;
  for (auto &n : nodes) {
    if (n.id == id)
      return &n;
  }
  return nullptr;
}

Node *FindNodeAtPoint(std::vector<Node> &nodes, Vector2 pos) {
  for (auto &n : nodes) {
    SDL_FRect r{n.UI_bounds.x, n.UI_bounds.y, n.UI_bounds.w, n.UI_bounds.h};
    if (is_point_in_rect(pos.x, pos.y, r))
      return &n;
  }
  return nullptr;
}

int main(int argc, char *argv[]) {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  TTF_Init();
  SDL_Window *window = SDL_CreateWindow("Node Editor", WINDOW_WIDTH,
                                        WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);

  AppContext app;
  app.font = TTF_OpenFont(app.fontPath.c_str(), app.fontSize);
  if (!app.font)
    return 1;
  BuildGlyphCache(app, renderer);

  std::vector<Node> nodes;
  std::vector<Connection> connections;

  // Attempt to load from binary file; fallback to defaults if file missing or
  // corrupt
  if (!NodeSerializer::LoadFromFile("save.data", nodes, connections)) {
    nodes.clear();
    nodes.push_back({0,
                     "Node 0",
                     "Variable",
                     std::nullopt,
                     std::nullopt,
                     0,
                     1,
                     {45, 45, 45, 255},
                     {100.0f, 100.0f, 150.0f, 100.0f}});
    nodes.push_back({1,
                     "Node 1",
                     "Consoe Log",
                     std::nullopt,
                     std::nullopt,
                     1,
                     0,
                     {45, 45, 45, 255},
                     {600.0f, 400.0f, 150.0f, 100.0f}});
  }

  // Restore the global ID counter based on the highest ID found in loaded data
  g_id_counter = -1;
  for (const auto &node : nodes) {
    if (node.id > g_id_counter)
      g_id_counter = node.id;
  }

  bool _running = true;
  SDL_Event event;
  MouseState mState;
  DraggingState dState;

  bool draw_command_pallete = false;
  std::string command_pallete_buffer;

  while (_running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT)
        _running = false;

      if (event.type == SDL_EVENT_MOUSE_MOTION) {
        mState.pos.x = event.motion.x;
        mState.pos.y = event.motion.y;

        // Apply movement vector if a node is actively being dragged
        if (dState.is_dragging_node) {
          for (auto &node : nodes) {
            if (node.id == dState.active_node_id) {
              node.UI_bounds.x = mState.pos.x - dState.drag_offset.x;
              node.UI_bounds.y = mState.pos.y - dState.drag_offset.y;
            }
          }
        }
      }

      // Handle raw character input when the command palette is active
      if (event.type == SDL_EVENT_TEXT_INPUT && draw_command_pallete) {
        command_pallete_buffer += event.text.text;
      }

      if (event.type == SDL_EVENT_KEY_DOWN) {
        const bool isCmdPressed = (SDL_GetModState() & SDL_KMOD_GUI) != 0;
        auto key = event.key.key;

        if (key == SDLK_ESCAPE)
          _running = false;

        // Delete node if mState.pos is_over a node
        if (key == SDLK_X)
          for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            SDL_FRect r{it->UI_bounds.x, it->UI_bounds.y, it->UI_bounds.w,
                        it->UI_bounds.h};
            if (is_point_in_rect(mState.pos.x, mState.pos.y, r)) {
              // If we are deleting a node that is currently selected.
              if (it->id == g_selected_node_id) {
                g_selected_node_id = -1;
              }
              nodes.erase(it);
              CompactNodeIds(nodes, connections);
              break;
            }
          }

        // Toggle command palette via Meta+P (macOS Cmd+P)
        if (isCmdPressed && key == SDLK_P) {
          draw_command_pallete = !draw_command_pallete;
          if (draw_command_pallete) {
            SDL_StartTextInput(window);
            command_pallete_buffer = "";
          } else {
            SDL_StopTextInput(window);
          }
        }

        if (draw_command_pallete) {
          // Finalize node creation or handle character deletion
          if (key == SDLK_RETURN) {
            NodePreset *best_match = nullptr;
            if (g_selected_node_id != -1 &&
                command_pallete_buffer.find("=") == 0) {
              // If user types "=0.5", set the selected node's value
              for (auto &n : nodes) {
                if (n.id == g_selected_node_id) {
                  n.custom_value = command_pallete_buffer.substr(1);
                  break;
                }
              }
              g_selected_node_id = -1;
            } else {
              // Simple find first search or "start_with" logic
              for (auto &preset : g_node_presets) {
                if (preset.type_name.find(command_pallete_buffer) !=
                    std::string::npos) {
                  best_match = &preset;
                  break;
                }
              }

              Node *existing = FindNodeAtPoint(nodes, mState.pos);
              if (existing) {
                g_selected_node_id = existing->id;
              } else if (best_match) {
                nodes.push_back(CreateNodeFromPreset(*best_match, mState));
                g_selected_node_id = nodes.back().id;
              } else {
                nodes.push_back(CreateNode(command_pallete_buffer, mState));
                g_selected_node_id = nodes.back().id;
              }
            }

            draw_command_pallete = false;
            SDL_StopTextInput(window);
          } else if (key == SDLK_BACKSPACE && !command_pallete_buffer.empty()) {
            command_pallete_buffer.pop_back();
          }
        }

        // Triggering build
        if (isCmdPressed && key == SDLK_B) {
          std::string generated_code =
              GraphCompiler::Compile(nodes, connections);
          printf("--- GENERATED CODE ---\n%s\n----------------------\n",
                 generated_code.c_str());

          // Optionally save to a file
          std::ofstream out("generated_script.cpp");
          out << generated_code;
          out.close();
        }
      }

      if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
          event.button.button == SDL_BUTTON_LEFT) {
        bool hit_something = false;

        // Hit Detection Pass 1: Node Pins (Connectors)
        for (auto &node : nodes) {
          for (int i = 0; i < node.output_count; i++) {
            Vector2 p = get_pin_pos(node, true, i);
            if (is_point_in_circle(mState.pos.x, mState.pos.y, p.x, p.y,
                                   10.0f)) {
              dState.is_dragging_connection = true;
              dState.active_node_id = node.id;
              dState.active_pin_id = i; // Add this field to DraggingState
              dState.start_pos = p;
              hit_something = true;
              break;
            }
          }
        }

        // Hit Detection Pass 2: Node Bodies (Movement)
        if (!hit_something) {
          for (int i = nodes.size() - 1; i >= 0; i--) {
            if (is_point_in_rect(mState.pos.x, mState.pos.y,
                                 {nodes[i].UI_bounds.x, nodes[i].UI_bounds.y,
                                  nodes[i].UI_bounds.w,
                                  nodes[i].UI_bounds.h})) {
              g_selected_node_id = nodes[i].id;
              dState.is_dragging_node = true;
              dState.active_node_id = nodes[i].id;
              dState.drag_offset = {mState.pos.x - nodes[i].UI_bounds.x,
                                    mState.pos.y - nodes[i].UI_bounds.y};
              hit_something = true;
              break;
            }
          }
        }

        if (!hit_something) {
          g_selected_node_id = -1;
        }
      }

      if (event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
          event.button.button == SDL_BUTTON_LEFT) {
        // Resolve connection logic if mouse released over an input pin
        if (dState.is_dragging_connection) {
          for (const auto &node : nodes) {
            for (int i = 0; i < node.input_count; i++) {
              Vector2 p = get_pin_pos(node, false, i);
              if (is_point_in_circle(mState.pos.x, mState.pos.y, p.x, p.y,
                                     10.0f)) {
                if (dState.active_node_id != node.id) {
                  connections.push_back({dState.active_node_id,
                                         dState.active_pin_id, node.id, i});
                }
              }
            }
          }
        }
        dState.is_dragging_connection = false;
        dState.is_dragging_node = false;
        dState.active_node_id = -1;
      }
    }

    SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    for (const auto &conn : connections) {
      Node *f = GetNodeById(nodes, conn.from_node_id);
      Node *t = GetNodeById(nodes, conn.to_node_id);
      if (f && t) {
        draw_bezier(renderer, get_pin_pos(*f, true, conn.from_pin_idx),
                    get_pin_pos(*t, false, conn.to_pin_idx));
      }
    }

    // Main Node Rendering Loop
    for (auto &node : nodes) {
      SDL_FRect r{node.UI_bounds.x, node.UI_bounds.y, node.UI_bounds.w,
                  node.UI_bounds.h};
      SDL_SetRenderDrawColor(renderer, node.color.r, node.color.g, node.color.b,
                             255);
      SDL_RenderFillRect(renderer, &r);

      float cy = node.UI_bounds.y + (node.UI_bounds.h / 2.0f);
      float cx = node.UI_bounds.x + (node.UI_bounds.w / 2.0f);
      // @Todo: some way to measure the text. so text always has a background
      DrawText(app, node.name, cx, cy, {0, 0, 0, 255}, renderer);

      // Draw custom node values underneath the name of variable.
      if (node.custom_value != "0.0f") {
        DrawText(app, "[" + node.custom_value + "]", cx, cy + 20,
                 {50, 50, 50, 255}, renderer);
      }

      // Render Input (Green) and Output (Blue) ports with hover scaling
      SDL_SetRenderDrawColor(renderer, 80, 220, 80, 255);
      for (int i = 0; i < node.input_count; ++i) {
        Vector2 p = get_pin_pos(node, false, i);
        float radius =
            is_point_in_circle(mState.pos.x, mState.pos.y, p.x, p.y, 10.0f)
                ? 7.0f
                : 4.0f;
        draw_circle(renderer, p.x, p.y, radius);
      }

      SDL_SetRenderDrawColor(renderer, 80, 80, 220, 255);
      for (int i = 0; i < node.output_count; i++) {
        Vector2 p = get_pin_pos(node, true, i);
        float radius =
            is_point_in_circle(mState.pos.x, mState.pos.y, p.x, p.y, 10.0f)
                ? 7.0f
                : 4.0f;
        draw_circle(renderer, p.x, p.y, radius);
      }

      // Highlight active node border while dragging
      SDL_SetRenderDrawColor(
          renderer,
          (dState.is_dragging_node && dState.active_node_id == node.id) ? 255
                                                                        : 100,
          100, 100, 255);
      SDL_RenderRect(renderer, &r);
    }

    // Draw active feedback line for port connections
    if (dState.is_dragging_connection) {
      SDL_SetRenderDrawColor(renderer, 255, 255, 100, 255);
      draw_bezier(renderer, dState.start_pos, mState.pos);
    }

    // Semi-transparent Overlay: Command Palette
    if (draw_command_pallete) {
      int rw, rh;
      SDL_GetRenderOutputSize(renderer, &rw, &rh);
      float cpw = 500.0f, cph = 25.0f;
      SDL_FRect cp_rect{(float)rw / 2.0f - (cpw / 2.0f),
                        (float)rh / 2.0f - (cph / 2.0f), cpw, cph};

      SDL_SetRenderDrawColor(renderer, 15, 15, 15, 255);
      SDL_RenderFillRect(renderer, &cp_rect);
      SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
      SDL_RenderRect(renderer, &cp_rect);

      Node *selected = GetNodeById(nodes, g_selected_node_id);
      if (selected) {
        std::string prefix = selected->name + " > ";
        DrawText(app, prefix, cp_rect.x + 5, cp_rect.y, {100, 200, 100, 255},
                 renderer);
        float offset = prefix.length() * app.charW;
        DrawText(app, command_pallete_buffer, cp_rect.x + 5.0f + offset,
                 cp_rect.y, {200, 200, 200, 255}, renderer);
      } else {
        DrawText(app, "?", cp_rect.x + 5, cp_rect.y, {200, 200, 200, 255},
                 renderer);
        DrawText(app, command_pallete_buffer, cp_rect.x + 16.0f, cp_rect.y,
                 {200, 200, 200, 255}, renderer);
      }

      // Draw search results
      float result_y = cp_rect.y + cp_rect.h + 5.0f;
      int count = 0;
      for (auto &preset : g_node_presets) {
        // Only show presets that match the current buffer
        if (preset.type_name.find(command_pallete_buffer) !=
            std::string::npos) {
          // Draw result background
          SDL_FRect res_rect = {cp_rect.x, result_y, cp_rect.w, 25.0f};
          SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
          SDL_RenderFillRect(renderer, &res_rect);

          // Draw preset color indicator
          SDL_FRect color_tag = {cp_rect.x + 2, result_y + 2, 4, 21};
          SDL_SetRenderDrawColor(renderer, preset.color.r, preset.color.g,
                                 preset.color.b, 255);
          SDL_RenderFillRect(renderer, &color_tag);

          DrawText(app, preset.type_name, cp_rect.x + 15, result_y + 4,
                   {180, 180, 180, 255}, renderer);

          result_y += 26.0f;
          count++;
          if (count > 5)
            break; // Limit results shown
        }
      }
    }

    SDL_RenderPresent(renderer);
  }

  // Binary persistence: Update save file upon clean exit
  NodeSerializer::SaveToFile("save.data", nodes, connections);

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}