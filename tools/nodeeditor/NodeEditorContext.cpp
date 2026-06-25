#include "NodeEditor.h"

namespace arch::tools {

NodeEditorContext::NodeEditorContext() {}

void NodeEditorContext::Update(MouseState mState, bool leftMouseDown) {
  static bool lastMouseDown = false;
  bool pressed = leftMouseDown && !leftMouseDown;
  bool released = !leftMouseDown && lastMouseDown;

  if (pressed) {
    bool hitPin = false;
    // Check pins first
    for (auto &node : _nodes) {
      int pinIdx;
      Vector2 pos;
      if (IsOverPin(node, mState.pos.x, mState.pos.y, true, pinIdx, pos)) {
        _isConnecting = true;
        _startNodeId = node.id;
        _startPinIdx = pinIdx;
        _connStartPos = pos;
        hitPin = true;
        break;
      }
    }

    if (!hitPin) {
      Node *hitNode = FindNodeAt(mState.pos.x, mState.pos.y);
      if (hitNode) {
        _activeNodeId = hitNode->id;
        _isDragging = true;
        _dragOffset = {mState.pos.x - hitNode->UI_bounds.x,
                       mState.pos.y - hitNode->UI_bounds.y};
      } else {
        _selectedNodeId = -1;
      }
    }
  }

  if (_isDragging && _activeNodeId != -1) {
    Node *node = nullptr;
    for (auto &n : _nodes)
      if (n.id == _activeNodeId)
        node = &n;
    if (node) {
      node->UI_bounds.x = mState.pos.x - _dragOffset.x;
      node->UI_bounds.y = mState.pos.y - _dragOffset.y;
    }
  }

  if (released) {
    if (_isConnecting) {
      for (auto &node : _nodes) {
        int pinIdx;
        Vector2 pos;
        if (IsOverPin(node, mState.pos.x, mState.pos.y, false, pinIdx, pos)) {
          if (node.id != _startNodeId) {
            _connections.push_back(
                {_startNodeId, _startPinIdx, node.id, pinIdx});
          }
        }
      }
    }
    _isDragging = false;
    _isConnecting = false;
    _activeNodeId = -1;
  }

  lastMouseDown = leftMouseDown;
}

void NodeEditorContext::Render(RenderAPI &api) {
  // Draw connections
  for (auto &conn : _connections) {
    // Find nodes and calculate pin positions (omitted for brevity, use your
    // get_pin_pos logic)
    // api.DrawLine(p1, p2, 2.0f, {150, 150, 150, 255});
  }

  for (auto &node : _nodes) {
    // Shape
    api.DrawRect(node.UI_bounds.x, node.UI_bounds.y, node.UI_bounds.w,
                 node.UI_bounds.h, node.color);

    // Border if selected
    if (node.id == _selectedNodeId) {
      api.DrawRectOutline(node.UI_bounds.x, node.UI_bounds.y, node.UI_bounds.w,
                          node.UI_bounds.h, {255, 255, 0, 255});
    }

    // Text
    api.DrawText(node.name, node.UI_bounds.x + 10, node.UI_bounds.y + 10,
                 {255, 255, 255, 255});

    // Pins (simplified example)
    for (int i = 0; i < node.input_count; ++i)
      api.DrawCircle(node.UI_bounds.x, node.UI_bounds.y + 30 + (i * 20), 5.0f,
                     {0, 255, 0, 255});
  }

  if (_isConnecting) {
    // api.DrawLine(m_connStartPos, m_mousePos, 2.0f, {255, 255, 0, 255});
  }
}

Node* NodeEditorContext::FindNodeAt(float x, float y) {
    for (int i = _nodes.size() - 1; i >= 0; i--) {
        auto& b = _nodes[i].UI_bounds;
        if (x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h) return &_nodes[i];
    }
    return nullptr;
}

bool NodeEditorContext::IsOverPin(const Node& node, float x, float y, bool isOutput, int& outPinIdx, Vector2& outPos) {
    // Implement your circle-collision check here
    return false;
}
} // namespace arch::tools
