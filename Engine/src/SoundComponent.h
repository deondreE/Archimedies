#pragma once

#include "Component.h"
#include "AudioEngine.h"

namespace Engine {
	class SoundComponent : public Engine::Component::Component
	{
	public:
		/*
		* Under the Hood is this an ECS "yes". You want to play a sound the user needs to know to add a Sound to the Entity.
		* They don't need to be foced to understand that that is a component.
		*/
		SoundComponent(const std::string& name, const std::filesystem::path& soundPath) 
			: Engine::Component::Component(name), _SoundPath(soundPath) 
		{
			_Sound = std::make_unique<Audio::Sound>();
			_Sound->Load(soundPath);
		}

		void ComponentAwake() override 
		{
			looping = _Sound->IsLooping();
			pitch = _Sound->GetPitch();
			volume = _Sound->GetVolume();
			pan = _Sound->GetPan();
		}
		
		void ComponentUpdate() override 
		{

		}

		void OnImGuiRender() override
		{

		}

		void Play()
		{
			// Use deafults unless the defaults change.
			_Sound->PlaySound();
		}

		float pitch = 0.0f;
		float volume = 0.0f;
		float pan = 0.0f;
		bool looping = false;
	private:
		std::filesystem::path _SoundPath;
		std::unique_ptr<Audio::Sound> _Sound = nullptr;
	};
}