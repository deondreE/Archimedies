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

// @TODO: Functions need bodies, -> Group of nodes: see excalidraw
// @TODO: Engine Calls need to be externed at the top of the file.
// @TODO: Body Nodes should be collapsable.
// @TODO: Script runner.

int g_id_counter = -1;
int g_selected_node_id = -1;
Node CreateNodeFromPreset(const NodePreset &preset, MouseState mState,
                          std::vector<Node> &nodes,
                          std::vector<Connection> &connections) {
  g_id_counter++;
  int parent_id = g_id_counter;
  Node parent = {g_id_counter,
                 preset.default_label,
                 preset.type_name,
                 std::nullopt,
                 std::nullopt,
                 preset.inputs,
                 preset.outputs,
                 Color(preset.color),
                 {mState.pos.x, mState.pos.y, 150.0f, 100.0f},
                 "",
                 preset.needs_body};

  if (preset.needs_body) {
    g_id_counter++;
    int child_id = g_id_counter;

    Node bodyNode = {child_id,
                     "Body Node",
                     preset.type_name,
                     std::nullopt,
                     std::nullopt,
                     1, // One input to receive the "body" connection
                     1,
                     {35, 35, 35, 255},
                     {mState.pos.x, mState.pos.y + 150.0f, 400.0f, 300.0f}};
    bodyNode.inner_node = BodyNodeInner<Node>(parent_id, {});
    Node innerDefault = {++g_id_counter,
                         "LOG",
                         "Utility",
                         std::nullopt,
                         std::nullopt,
                         1,
                         0,
                         {180, 100, 50, 255},
                         {mState.pos.x + 100, mState.pos.y + 200, 120, 60}};
    bodyNode.inner_node->nodes.push_back(innerDefault);

    nodes.push_back(bodyNode);

    connections.push_back({parent_id, 999, child_id, 0});
  }

  return parent;
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

float GetDisplayScale(SDL_Window *window, SDL_Renderer *renderer) {
  int win_w, rend_w;
  SDL_GetWindowSize(window, &win_w, NULL);
  SDL_GetCurrentRenderOutputSize(renderer, &rend_w, NULL);
  return (win_w > 0) ? (float)rend_w / (float)win_w : 1.0f;
}

// 1. Create a helper function above main() or at the top of the file
void DrawSingleNode(SDL_Renderer *renderer, AppContext &app, Node &node,
                    MouseState &mState, DraggingState &dState) {
  SDL_FRect r{node.UI_bounds.x, node.UI_bounds.y, node.UI_bounds.w,
              node.UI_bounds.h};

  if (node.name == "Body Node") {
    SDL_SetRenderDrawColor(renderer, 15, 15, 20, 255);
    SDL_RenderFillRect(renderer, &r);
    SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
    SDL_RenderRect(renderer, &r);

    // Draw the Container Label
    DrawText(app, "BODY: " + node.internalFunction, node.UI_bounds.x + 10,
             node.UI_bounds.y - 25, {100, 150, 255, 255}, renderer);

    SDL_FColor iColor = {1.0f, 0.65f, 0.0f, 1.0f};
    // Render Internal Passthrough Pins
    for (int i = 0; i < node.input_count; i++) {
      Vector2 p = get_pin_pos(node, false, i);
      
      draw_circle(renderer, p.x, p.y, 6.0f, iColor);
    }

    SDL_FColor wColor = {1.0f, 0.0f, 1.0f, 1.0f};
    for (int i = 0; i < node.output_count; i++) {
      Vector2 p = get_pin_pos(node, true, i);
      draw_circle(renderer, p.x, p.y, 6.0f, wColor);
    }

    // RECURSION: Draw Inner Nodes
    if (node.inner_node.has_value()) {
      for (auto &inner : node.inner_node->nodes) {
        DrawSingleNode(renderer, app, inner, mState, dState);
      }
    }
  }
  // Standard Node Style
  else {
    SDL_SetRenderDrawColor(renderer, node.color.r, node.color.g, node.color.b,
                           255);
    SDL_RenderFillRect(renderer, &r);

    DrawText(app, node.name, node.UI_bounds.x + 10, node.UI_bounds.y + 10,
             {255, 255, 255, 255}, renderer);

    if (node.custom_value != "0.0f") {
      const float M_WIDTH = 10.0f;
      const float M_HEIGHT = 20.0f;
      
      float text_w = (float)node.custom_value.length() * M_WIDTH;
      float text_needed_w = text_w + 20.0f;

      if (text_needed_w > node.UI_bounds.w) {
        node.UI_bounds.w = text_needed_w;
      }
      
      float text_x = node.UI_bounds.x + 10.0f;
      float text_y =
          node.UI_bounds.y + (node.UI_bounds.h - M_HEIGHT) / 2.0f;

      DrawText(app, "[" + node.custom_value + "]", text_x, text_y,
               {186, 199, 219, 255}, renderer);
    }

    if (node.name == "COMMENT") {
      DrawText(app, node.name, node.UI_bounds.x + 10, node.UI_bounds.y + 10, {0, 0, 0, 255}, renderer);

      SDL_SetRenderDrawColor(renderer, 122, 122, 122, 255);
      SDL_FRect resize_rect{node.UI_bounds.x + node.UI_bounds.w - 10.0f,
                            node.UI_bounds.y + node.UI_bounds.h - 10.0f, 10.0f,
                            10.0f};
      SDL_RenderFillRect(renderer, &resize_rect);
    }

    // Draw normal pins
    SDL_FColor iColor = {0.31f, 0.86f, 0.31f, 1.0f};
    for (int i = 0; i < node.input_count; ++i) {
      Vector2 p = get_pin_pos(node, false, i);
      draw_circle(renderer, p.x, p.y, 4.0f, iColor);
    }

    SDL_FColor oColor = {0.31f, 0.31f, 0.86f, 1.0f};
    for (int i = 0; i < node.output_count; ++i) {
      Vector2 p = get_pin_pos(node, true, i);
      draw_circle(renderer, p.x, p.y, 4.0f, oColor);
    }
  }

  // Border highlight
  SDL_SetRenderDrawColor(
      renderer, (dState.active_node_id == node.id) ? 255 : 50, 50, 50, 255);
  SDL_RenderRect(renderer, &r);
}

int main(int argc, char *argv[]) {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  TTF_Init();
  SDL_Window *window =
      SDL_CreateWindow("Node Editor", WINDOW_WIDTH, WINDOW_HEIGHT,
                       SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
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

  float dpi_scale = GetDisplayScale(window, renderer);
  SDL_SetRenderScale(renderer, dpi_scale, dpi_scale);
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
              float new_x = mState.pos.x - dState.drag_offset.x;
              float new_y = mState.pos.y - dState.drag_offset.y;
              float dx = new_x - node.UI_bounds.x;
              float dy = new_y - node.UI_bounds.y;
              node.UI_bounds.x = new_x;
              node.UI_bounds.y = new_y;

              if (node.inner_node.has_value()) {
                for (auto &inner : node.inner_node->nodes) {
                  inner.UI_bounds.x += dx;
                  inner.UI_bounds.y += dy;
                }
              }
            }
          }
        } else if (dState.is_resizing_node) {
          for (auto &node : nodes) {
            if (node.id == dState.active_node_id) {
              float new_w = mState.pos.x - node.UI_bounds.x;
              float new_h = mState.pos.y - node.UI_bounds.y;

              node.UI_bounds.w = (new_w > 50.0f) ? new_w : 50.0f;
              node.UI_bounds.h = (new_h > 50.0f) ? new_h : 50.0f;
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
        if (key == SDLK_X) {
          for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            SDL_FRect r{it->UI_bounds.x, it->UI_bounds.y, it->UI_bounds.w,
                        it->UI_bounds.h};

            if (is_point_in_rect(mState.pos.x, mState.pos.y, r)) {
              int id_to_delete = it->id;
              int body_node_id = -1;
              if (it->needs_body) {
                for (const auto &conn : connections) {
                  if (conn.from_node_id == id_to_delete &&
                      conn.from_pin_idx == 999) {
                    body_node_id = conn.to_node_id;
                    break;
                  }
                }
              }

              if (id_to_delete == g_selected_node_id ||
                  body_node_id == g_selected_node_id) {
                g_selected_node_id = -1;
              }

              std::erase_if(nodes, [id_to_delete, body_node_id](const Node &n) {
                return n.id == id_to_delete || n.id == body_node_id;
              });

              CompactNodeIds(nodes, connections);
              break;
            }
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
                nodes.push_back(CreateNodeFromPreset(*best_match, mState, nodes,
                                                     connections));
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

          if (node.needs_body) {
            Vector2 p = get_pin_pos(node, true, 0, true);
            if (is_point_in_circle(mState.pos.x, mState.pos.y, p.x, p.y,
                                   10.0f)) {
              dState.is_dragging_connection = true;
              dState.active_node_id = node.id;
              dState.active_pin_id = 999; // Add this field to DraggingState
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
              if (nodes[i].name == "COMMENT" &&
                  GetNodeRegion(mState.pos, nodes[i].UI_bounds, 20.0f) ==
                      NodeRegion::BOTTOM_RIGHT) {
                dState.is_dragging_node = false;
                dState.is_resizing_node = true;
                dState.active_node_id = nodes[i].id;
              } else {
                g_selected_node_id = nodes[i].id;
                dState.is_dragging_node = true;
                dState.active_node_id = nodes[i].id;
                dState.drag_offset = {mState.pos.x - nodes[i].UI_bounds.x,
                                      mState.pos.y - nodes[i].UI_bounds.y};
                if (nodes[i].needs_body) {
                  for (auto &conn : connections) {
                    if (conn.from_node_id == nodes[i].id &&
                        conn.from_pin_idx == 999) {
                      Node *body = GetNodeById(nodes, conn.to_node_id);
                      if (body) {
                        body->UI_bounds.x +=
                            (mState.pos.x - dState.drag_offset.x) -
                            nodes[i].UI_bounds.x;
                        body->UI_bounds.y +=
                            (mState.pos.y - dState.drag_offset.y) -
                            nodes[i].UI_bounds.y;
                      }
                    }
                  }
                }
              }
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
                  connections.push_back(
                      {dState.active_node_id, dState.active_pin_id, node.id,
                       i}); // is_body could be looking for 999
                }
              }
            }
          }
        }
        dState.is_dragging_connection = false;
        dState.is_dragging_node = false;
        dState.is_resizing_node = false;
        dState.active_node_id = -1;
      }

      if (event.type == SDL_EVENT_WINDOW_DISPLAY_CHANGED ||
          event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        float new_scale = GetDisplayScale(window, renderer);
        SDL_SetRenderScale(renderer, new_scale, new_scale);
        BuildGlyphCache(app, renderer);
      }
    }

    SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    for (const auto &conn : connections) {
      Node *f = GetNodeById(nodes, conn.from_node_id);
      Node *t = GetNodeById(nodes, conn.to_node_id);
      if (f && t) {
        bool is_body = (conn.from_pin_idx == 999);
        draw_bezier(renderer,
                    get_pin_pos(*f, true, is_body ? 0 : conn.from_pin_idx),
                    get_pin_pos(*t, false, conn.to_pin_idx), 2.0f);
      }
    }

    // Main rendering
    for (auto &node : nodes) {
      DrawSingleNode(renderer, app, node, mState, dState);
    }

    // Draw active feedback line for port connections
    if (dState.is_dragging_connection) {
      SDL_SetRenderDrawColor(renderer, 255, 255, 100, 255);
      draw_bezier(renderer, dState.start_pos, mState.pos);
    }

    // Semi-transparent Overlay: Command Palette
    if (draw_command_pallete) {
      SDL_Window *win = SDL_GetRenderWindow(renderer);
      int rw, rh;
      SDL_GetWindowSize(win, &rw, &rh);
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
