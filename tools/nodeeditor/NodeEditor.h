#pragma once
#include <functional>
#include <string>

namespace arch::tools {

struct Connection {
  int from_node_id;
  int from_pin_idx;
  int to_node_id;
  int to_pin_idx;
};

struct Color {
  float r, g, b, a;
};

struct Vector2 {
  float x, y;
};

struct MouseState {
  Vector2 pos;
};

struct Rect {
  float x, y, w, h;
};

/// This refers to a body node, the nodes inside of that body
template <typename T> struct BodyNodeInner {
  int parent_id;
  std::vector<T> nodes;
};

/** Default node type */
struct Node {
  int id;
  std::string name;
  std::string internalFunction;

  int input_count = 1;
  int output_count = 1;

  Color color;
  Rect UI_bounds;
  std::string custom_value = "0.0f";
  bool needs_body = false;
  std::optional<BodyNodeInner<Node>> inner_node = std::nullopt;
};

struct RenderAPI {
  std::function<void(float x, float y, float w, float h, Color c)> DrawRect;
  std::function<void(float x, float y, float w, float h, Color c)>
      DrawRectOutline;
  std::function<void(float x, float y, float r, Color c)> DrawCircle;
  std::function<void(const std::string &text, float x, float y, Color c)>
      DrawText;
  std::function<void(Vector2 start, Vector2 end, float thickness, Color c)>
      DrawLine;
};

class NodeEditorContext {
public:
  NodeEditorContext();

  void Update(MouseState mState, bool isMouseDown);
  void Render(RenderAPI &api);

  void AddNode(const Node &node);
  void DeleteNode(int nodeId);

  std::vector<Node> &GetNodes() { return _nodes; }
  std::vector<Connection> &GetConnections() { return _connections; }

private:
  std::vector<Node> _nodes;
  std::vector<Connection> _connections;

  // Interaction State
  int _activeNodeId = -1;
  int _selectedNodeId = -1;
  bool _isDragging = false;
  bool _isConnecting = false;
  Vector2 _dragOffset = {0, 0};

  int _startNodeId = 0;
  int _startPinIdx = 0;
  Vector2 _connStartPos = {0, 0};

  Node *FindNodeAt(float x, float y);
  bool IsOverPin(const Node &node, float x, float y, bool isOutput,
                 int &outPinIdx, Vector2 &outPos);
};

} // namespace arch::tools
