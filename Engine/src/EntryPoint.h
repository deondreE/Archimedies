#pragma once
#include "archpch.h"
#include "Application.h"

/**
 * This function must be defined in the Client (Sandbox)
 * to return a new instance of their specific application class.
 */
extern Engine::Application* Engine::CreateApplication();

int main(int argc, char** argv)
{
    // Optional: Initialize Engine Logging/Systems here

    // Create the application instance from the Client
    auto app = Engine::CreateApplication();

    // Run the main engine loop (Win32 messages + DirectX Render)
    app->Run();

    // Clean up
    delete app;

    return 0;
}