using Engine.Managed;
using System;

public class TestScript : ScriptComponent
{
    private float _elapsed = 0f;

    public override void OnStart()
    {
        Console.WriteLine("[TestScript] OnStart called - script loaded successfully!");
    }

    public override void OnUpdate(float deltaTime)
    {
        _elapsed += deltaTime;

        // Print roughly once a second so you don't flood the console
        if (_elapsed >= 1.0f)
        {
            Console.WriteLine($"[TestScript] Tick - deltaTime: {deltaTime:F4}");
            _elapsed = 0f;
        }
    }
}