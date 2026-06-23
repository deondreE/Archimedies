#pragma once

#include "types.h"
#include <iostream>
#include <string>

#include "tiny_gltf_v3.h"

namespace arch::core::kernel {
struct Mesh {
  tg3_model model;
};

class MeshLoader {
public:
  MeshLoader() = default;
  ~MeshLoader() {
    for (auto &[path, model] : _models) {
      delete &model;
    }
  }

  bool LoadMesh(const std::string &model_path) {
    if (_models.find(model_path) != _models.end()) {
      return true;
    }
    tg3_model model;
    tg3_error_stack errors;
    tg3_parse_options opts;
    tg3_parse_options_init(&opts);
    tg3_error_stack_init(&errors);

    tg3_error_code err =
        tg3_parse_file(&model, &errors, model_path.c_str(), 10, &opts);
    if (err != TG3_OK) {
      for (u32 i = 0; i < errors.count; ++i) {
        // @TODO: Logger
        std::cerr << "Loader Error: "
                  << (errors.entries[i].message ? errors.entries[i].message
                                                : "Unknown")
                  << std::endl;
      }
      tg3_error_stack_free(&errors);
      return false;
    }
    _models[model_path] = model;

    tg3_error_stack_free(&errors);
    return true;
  }

  void UnloadMesh(const std::string &model_path) {
    auto it = _models.find(model_path);
    if (it == _models.end()) {
      tg3_model_free(&it->second);
      _models.erase(it);
    }
  }

  tg3_model *GetModel(const std::string &model_path) {
    auto it = _models.find(model_path);
    if (it != _models.end()) {
      return &it->second;
    }
    return nullptr;
  }

private:
  std::unordered_map<std::string, tg3_model> _models;
};
} // namespace arch::core::kernel