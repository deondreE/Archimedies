#pragma once
#include "Layer.h"
#include "Camera.h"
#include "Renderer.h"
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#include "Scene.h"
#include "WindowEvents.h"
#include "imgui.h"
#include "SceneSerializer.h"
#include "ScriptManager.h"

class SandboxLayer : public Engine::Layer {
public:
    SandboxLayer(Engine::Scene* scene, ID3D11Device* device, const std::wstring& workingDir, int width, int height)
        : Layer("Sandbox"), _Scene(scene), _Device(device), _WorkingDir(workingDir), _Width(width), _Height(height) {
        std::filesystem::path root(_WorkingDir);

        Cora::ScriptManager::Config scriptConfig;
        scriptConfig.enableWatcher = true;

        scriptConfig.managedDllPath = root / "Engine.Managed.dll";
        scriptConfig.scriptDir = root / "Scripts";

        _ScriptManager.InitAsync(scriptConfig);
	}

    virtual void OnAttach() override {
        auto& shaders = Engine::Renderer::GetShaderLibrary();
        auto shader = shaders.Load(_Device, "Basic", _WorkingDir + L"/Shaders/Basic.hlsl");
        auto material = std::make_shared<Engine::Material>(shader);

        std::vector<Engine::Vertex> verts = {
            {{ 0.0f,  0.5f, 0.0f}, {1, 1, 1, 1}, {0.5f, 0.0f}, {0, 0, -1}},
            {{ 0.5f, -0.5f, 0.0f}, {1, 1, 1, 1}, {1.0f, 1.0f}, {0, 0, -1}},
            {{-0.5f, -0.5f, 0.0f}, {1, 1, 1, 1}, {0.0f, 1.0f}, {0, 0, -1}} };
        std::vector<uint32_t> indices = { 0, 1, 2 };
        auto mesh = Engine::Mesh::Create(_Device, verts, indices);

        for (int i = 0; i < 10; i++) {
            auto& e = _Scene->CreateEntity("Sensor_" + std::to_string(i));
            e.Position = Engine::Math::Vec3((float)i * 2.0f, 0.0f, 10.0f);
            e.Mesh = mesh;
            e.Material = material;
        }

        float aspect = (float)_Width / (float)_Height;
        _Camera = std::make_unique<Engine::Camera>(45.0f, aspect, 0.1f, 1000.0f);

        Engine::SceneSerializer::Serialize(*_Scene, "Sandbox.scene");
    }

    virtual void OnUpdate(Engine::Timestep ts) override {
        _Camera->OnUpdate(ts);
        _ScriptManager.ScriptTick(ts.GetSeconds());
    }

    virtual void OnRender() override {
        Engine::Renderer::BeginScene(_Camera->GetViewProjection());
        for (const auto& entity : _Scene->GetEntities()) {
            Engine::Renderer::Submit(entity);
        }
        Engine::Renderer::EndScene();
    }

    virtual void OnEvent(Engine::Event& e) override {
        Engine::EventDispatcher dispatcher(e);
        dispatcher.Dispatch<Engine::WindowResizeEvent>([this](Engine::WindowResizeEvent& re) {
            _Camera->OnResize(re.GetWidth(), re.GetHeight());
            return false;
        });
    }

    virtual void OnDetach() override {
        _ScriptManager.Shutdown();
    }
private:
    Engine::Scene* _Scene;
    ID3D11Device* _Device;
    std::wstring _WorkingDir;
    int _Width, _Height;
    std::unique_ptr<Engine::Camera> _Camera;
    Cora::ScriptManager _ScriptManager;
};
