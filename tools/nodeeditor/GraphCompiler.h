#include "apptypes.h"
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class GraphCompiler {
public:
  static std::string Compile(const std::vector<Node> &nodes,
                             const std::vector<Connection> &connections) {
    if (nodes.empty())
      return "";

    const size_t node_count = nodes.size();
    std::unordered_map<int, size_t> id_to_index;
    for (size_t i = 0; i < node_count; ++i)
      id_to_index[nodes[i].id] = i;

    // Separate execution flow (pins >= 10) and track dependencies
    std::unordered_map<int, std::vector<size_t>> block_children;
    std::vector<bool> is_child_node(node_count, false);

    for (const auto &conn : connections) {
      if (conn.to_pin_idx >= 10 && id_to_index.contains(conn.to_node_id)) {
        block_children[conn.from_node_id].push_back(
            id_to_index[conn.to_node_id]);
        is_child_node[id_to_index[conn.to_node_id]] = true;
      }
    }

    // Helper to solve dependencies within a specific scope
    auto GetSortedScope = [&](const std::vector<size_t> &indices) {
      std::vector<size_t> sorted = indices;
      bool changed = true;
      while (changed) {
        changed = false;
        for (const auto &conn : connections) {
          if (conn.to_pin_idx < 10) { // Data connection
            auto it_from = std::find(sorted.begin(), sorted.end(),
                                     id_to_index[conn.from_node_id]);
            auto it_to = std::find(sorted.begin(), sorted.end(),
                                   id_to_index[conn.to_node_id]);
            if (it_from != sorted.end() && it_to != sorted.end() &&
                it_from > it_to) {
              std::swap(*it_from, *it_to);
              changed = true;
            }
          }
        }
      }
      return sorted;
    };

    auto CompileNode = [&](auto &self, size_t idx, int indent_level,
                           int parent_id) -> std::string {
      const Node &node = nodes[idx];
      std::string indent(indent_level * 4, ' ');

      const NodePreset *preset = nullptr;
      for (const auto &p : g_node_presets) {
        if (p.type_name == node.internalFunction) {
          preset = &p;
          break;
        }
      }
      if (!preset)
        return indent + "// Missing: " + node.name + "\n";

      std::string line = preset->code_template;
      auto replace = [&](std::string k, std::string v) {
        size_t p = 0;
        while ((p = line.find(k, p)) != std::string::npos) {
          line.replace(p, k.length(), v);
          p += v.length();
        }
      };

      // Replace attributes
      replace("{id}", std::to_string(node.id));
      replace("{out}", "v" + std::to_string(node.id));
      replace("{val}", node.custom_value.empty() ? "0" : node.custom_value);

      // Replace data inputs
      for (const auto &conn : connections) {
        if (conn.to_node_id == node.id && conn.to_pin_idx < 10) {
          replace("{" + std::to_string(conn.to_pin_idx) + "}",
                  "v" + std::to_string(conn.from_node_id));
        }
      }

      // Default disconnected pins
      size_t s;
      while ((s = line.find('{')) != std::string::npos) {
        size_t e = line.find('}', s);
        if (e == std::string::npos)
          break;
        if (isdigit(line[s + 1]))
          line.replace(s, (e - s) + 1, "0.0f");
        else
          break;
      }

      // Recursive body handling
      if (line.find("{}") != std::string::npos) {
        std::string body = "";
        if (block_children.contains(node.id)) {
          auto sorted_children = GetSortedScope(block_children[node.id]);
          for (size_t child_idx : sorted_children) {
            body += self(self, child_idx, indent_level + 1, node.id);
          }
        }
        size_t b_pos = line.find("{}");
        line.replace(b_pos, 2, "{\n" + body + indent + "}");
      }

      return indent + line + "\n";
    };

    // Final Assembly
    std::stringstream ss;
    ss << "#include <iostream>\n#include <vector>\n\nvoid ProcessGraph() {\n";

    std::vector<size_t> roots;
    for (size_t i = 0; i < node_count; ++i)
      if (!is_child_node[i])
        roots.push_back(i);

    for (size_t root_idx : GetSortedScope(roots)) {
      ss << CompileNode(CompileNode, root_idx, 1, -1);
    }

    ss << "}\n";
    return ss.str();
  }
};
