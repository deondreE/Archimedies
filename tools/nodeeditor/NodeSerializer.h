#pragma once

#include "apptypes.h"
#include <cstddef>
#include <fstream>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

class NodeSerializer {
public:
  static bool SaveToFile(const std::string &filename,
                         const std::vector<Node> &nodes,
                         const std::vector<Connection> &connections) {
    std::ofstream out(filename, std::ios::binary);
    if (!out)
      return false;

    int version = 1;
    out.write(reinterpret_cast<char *>(&version), sizeof(version));

    size_t nodeCount = nodes.size();
    out.write(reinterpret_cast<char *>(&nodeCount), sizeof(nodeCount));
    for (const auto &node : nodes) {
      WriteNode(out, node);
    }

    size_t connCount = connections.size();
    out.write(reinterpret_cast<char *>(&connCount), sizeof(connCount));
    for (const auto &conn : connections) {
      out.write(reinterpret_cast<const char *>(&conn), sizeof(Connection));
    }

    return true;
  }

  static bool LoadFromFile(const std::string &filename,
                           std::vector<Node> &outNodes,
                           std::vector<Connection> &outConns) {
    std::ifstream in(filename, std::ios::binary);
    if (!in)
      return false;

    int version = 0;
    in.read(reinterpret_cast<char *>(&version), sizeof(version));

    size_t nodeCount = 0;
    in.read(reinterpret_cast<char *>(&nodeCount), sizeof(nodeCount));

    outNodes.clear();
    for (size_t i = 0; i < nodeCount; ++i) {
      outNodes.push_back(ReadNode(in));
    }

    size_t connCount = 0;
    if (in.read(reinterpret_cast<char *>(&connCount), sizeof(connCount))) {
      outConns.clear();
      for (size_t i = 0; i < connCount; ++i) {
        Connection conn;
        in.read(reinterpret_cast<char *>(&conn), sizeof(Connection));
        outConns.push_back(conn);
      }
    }

    return true;
  }

private:
  static void WriteString(std::ostream &os, const std::string &str) {
    size_t size = str.size();
    os.write(reinterpret_cast<char *>(&size), sizeof(size));
    os.write(str.data(), size);
  }

  static std::string ReadString(std::istream &is) {
    size_t size = 0;
    is.read(reinterpret_cast<char *>(&size), sizeof(size));
    if (size > 1000000)
      return ""; // Safety cap
    std::string str(size, '\0');
    is.read(&str[0], size);
    return str;
  }

  static void WritePin(std::ostream &os, const Pin &pin) {
    os.write(reinterpret_cast<const char *>(&pin.id), sizeof(pin.id));
    os.write(reinterpret_cast<const char *>(&pin.nodeId), sizeof(pin.nodeId));
    WriteString(os, pin.name);
    os.write(reinterpret_cast<const char *>(&pin.direction),
             sizeof(pin.direction));
    os.write(reinterpret_cast<const char *>(&pin.dataType),
             sizeof(pin.dataType));
  }

  static Pin ReadPin(std::istream &is) {
    Pin pin;
    is.read(reinterpret_cast<char *>(&pin.id), sizeof(pin.id));
    is.read(reinterpret_cast<char *>(&pin.nodeId), sizeof(pin.nodeId));
    pin.name = ReadString(is);
    is.read(reinterpret_cast<char *>(&pin.direction), sizeof(pin.direction));
    is.read(reinterpret_cast<char *>(&pin.dataType), sizeof(pin.dataType));
    return pin;
  }

  static void WriteNode(std::ostream &os, const Node &node) {
    os.write(reinterpret_cast<const char *>(&node.id),
             sizeof(node.id)); // Added &
    WriteString(os, node.name);
    WriteString(os, node.internalFunction);
    os.write(reinterpret_cast<const char *>(&node.color), sizeof(node.color));
    os.write(reinterpret_cast<const char *>(&node.UI_bounds),
             sizeof(node.UI_bounds));

    os.write(reinterpret_cast<const char *>(&node.input_count),
             sizeof(node.input_count));
    os.write(reinterpret_cast<const char *>(&node.output_count),
             sizeof(node.output_count));
    WriteString(os, node.custom_value);

    os.write(reinterpret_cast<const char*>(&node.needs_body), sizeof(node.needs_body));

    bool hasInputs = node.inputs.has_value();
    os.write(reinterpret_cast<const char *>(&hasInputs), sizeof(hasInputs));
    if (hasInputs) {
      size_t count = node.inputs->size();
      os.write(reinterpret_cast<const char *>(&count), sizeof(count));
      for (const auto &pin : *node.inputs)
        WritePin(os, pin);
    }

    bool hasOutputs = node.outputs.has_value();
    os.write(reinterpret_cast<const char *>(&hasOutputs),
             sizeof(hasOutputs)); // Added &
    if (hasOutputs) {
      size_t count = node.outputs->size();
      os.write(reinterpret_cast<const char *>(&count), sizeof(count));
      for (const auto &pin : *node.outputs)
        WritePin(os, pin);
    }
  }

  static Node ReadNode(std::istream &is) {
    Node node;
    is.read(reinterpret_cast<char *>(&node.id), sizeof(node.id));
    node.name = ReadString(is);
    node.internalFunction = ReadString(is);
    is.read(reinterpret_cast<char *>(&node.color), sizeof(node.color));
    is.read(reinterpret_cast<char *>(&node.UI_bounds), sizeof(node.UI_bounds));
    is.read(reinterpret_cast<char *>(&node.input_count),
            sizeof(node.input_count));
    is.read(reinterpret_cast<char *>(&node.output_count),
            sizeof(node.output_count));
    node.custom_value = ReadString(is);

    is.read(reinterpret_cast<char *>(&node.needs_body), sizeof(node.needs_body));

    bool hasInputs = false;

    is.read(reinterpret_cast<char *>(&hasInputs),
            sizeof(hasInputs)); // Fixed target
    if (hasInputs) {
      size_t count = 0;
      is.read(reinterpret_cast<char *>(&count), sizeof(count)); // Fixed target
      std::vector<Pin> pins(count);
      for (size_t i = 0; i < count; ++i)
        pins[i] = ReadPin(is);
      node.inputs = pins;
    }

    bool hasOutputs = false;
    is.read(reinterpret_cast<char *>(&hasOutputs), sizeof(hasOutputs));
    if (hasOutputs) {
      size_t count = 0;
      is.read(reinterpret_cast<char *>(&count), sizeof(count));
      std::vector<Pin> pins(count);
      for (size_t i = 0; i < count; ++i)
        pins[i] = ReadPin(is);
      node.outputs = pins;
    }

    return node;
  }
};
