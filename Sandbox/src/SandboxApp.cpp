#include "Application.h"
#include "EntryPoint.h"
#include "Renderer.h"
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#include "Camera.h"

class Sandbox : public Engine::Application {
public:
    Sandbox() : Application(MakeSpec()) {}

    virtual void OnInit() override {
        auto& shaders = Engine::Renderer::GetShaderLibrary();
        auto basicShader = shaders.Load(GetDevice(), "Basic", _Specification.WorkingDirectory + L"/Shaders/Basic.hlsl");
        
        auto texture = Engine::Texture2D::Create(GetDevice(), "../Sandbox/Assets/test.png");
        auto material = std::make_shared<Engine::Material>(basicShader, texture);

        std::vector<Engine::Vertex> verts = {
            { {  0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };
        std::vector<uint32_t> indices = { 0, 1, 2 };
        auto mesh = Engine::Mesh::Create(GetDevice(), verts, indices);

        for (int i = 0; i < 10; i++) {
            auto& e = _ActiveScene->CreateEntity("Sensor_" + std::to_string(i));
            e.Position = Engine::Math::Vec3(static_cast<float>(i) * 2.0f, 0.0f, 10.0f);
            e.Mesh = mesh;
            e.Material = material;
        }

        float aspect = (float)_Width / (float)_Height;
        _Camera = std::make_unique<Engine::Camera>(45.0f, aspect, 0.1f, 1000.0f);
    }

    virtual void OnUpdate(Engine::Timestep ts) override {
        _Camera->OnUpdate(ts);
    }

    virtual void OnViewportResize(uint32_t width, uint32_t height) override {
        _Camera->OnResize(width, height);
    }

    virtual void OnRender() override {
        Engine::Renderer::BeginScene(_Camera->GetViewProjection());

        for (const auto& entity : _ActiveScene->GetEntities()) {
            Engine::Renderer::Submit(entity);
        }

        Engine::Renderer::EndScene();
    }

private:
    std::unique_ptr<Engine::Camera> _Camera;

    static Engine::ApplicationSpecification MakeSpec() {
        Engine::ApplicationSpecification spec;
        spec.Name = L"Archimedies Engineering Sandbox";
        spec.WorkingDirectory = L"../Sandbox";
        spec.Width = 1280;
        spec.Height = 720;
        return spec;
    }
};

Engine::Application* Engine::CreateApplication() {
    return new Sandbox();
}