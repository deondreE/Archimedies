#pragma once

#include <nlohmann/json.hpp>
#include <fstream>
#include "Scene.h"

namespace Engine {
    using json = nlohmann::json;
    class SceneSerializer {
    public:
        static void Serialize(const Scene& scene, const std::string& filepath) {
            if (filepath.empty()) return;

            json root;
            root["SceneName"] = "Archimedies Scene";
            root["Entities"] = json::array();

            for (const auto& entity : scene.GetEntities()) {
                json e;
                e["ID"] = (uint64_t)entity.ID;
                e["Name"] = entity.Name;

                e["Transform"] = {
                    {"Position", {entity.Position.x, entity.Position.y, entity.Position.z}},
                    {"Rotation", {entity.Rotation.x, entity.Rotation.y, entity.Rotation.z}},
                    {"Scale",    {entity.Scale.x,    entity.Scale.y,    entity.Scale.z}}
                };

                if (entity.Mesh)
                    e["MeshPath"] = entity.Mesh->GetPath(); 

                root["Entities"].push_back(e);
            }

            std::ofstream fout(filepath);
            if (fout.is_open()) {
                // Use dump(4) for pretty-printing, making it diff-friendly
                fout << root.dump(4);
                fout.close();
            }
        }
    };
}