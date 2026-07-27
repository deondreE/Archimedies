#include "Application.h"
#include "EntryPoint.h"
#include "SandboxLayer.h"
#include "EditorLayer.h"

class Sandbox : public Engine::Application {
public:
    Sandbox() : Application(MakeSpec()) {}

    virtual void OnInit() override {
        PushLayer(new SandboxLayer(_ActiveScene.get(), GetDevice(), _Specification.WorkingDirectory, _Width, _Height));
        PushOverlay(new Engine::EditorLayer(_ActiveScene.get()));
    }
private:
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