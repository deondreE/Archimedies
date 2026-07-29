#pragma once
#include "archpch.h"
#include <filesystem>
#include <xaudio2.h>
#include <concepts>
#include <numbers>
#include <memory>
#include <algorithm>
#include <random>
#include <vector>

namespace Engine::Audio {

    // --- Concepts & Macros ---
    template<typename T>
    concept FloatingPoint = std::is_floating_point_v<T>;

#ifndef fourccRIFF
#ifdef _XBOX
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#else
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#endif
#endif

#define MAX_CHANNELS_PAN 16

    class SoundCallback : public IXAudio2VoiceCallback {
    public:
        void OnStreamEnd() override {}
        void OnVoiceProcessingPassStart(UINT32) override {}
        void OnVoiceProcessingPassEnd() override {}
        void OnVoiceError(void*, HRESULT) override {}
        void OnBufferStart(void*) override {}
        void OnBufferEnd(void* pContext) override {
            if (pContext) {
                *static_cast<bool*>(pContext) = false;
            }
        }
        void OnLoopEnd(void*) override {}
    };

    enum class BusType { Master, SFX, Music };

    struct AudioBufferResource {
        WAVEFORMATEXTENSIBLE Format = { 0 };
        std::vector<BYTE> Data;
    };

    inline SoundCallback g_VoiceCallback;

    class Sound;

    class AudioEngine {
    public:
        static constexpr size_t MAX_VOICES = 32;

        static void Init() { Get().instance_Init(); }
        static IXAudio2* GetContext() { return Get()._XAudio; }
        static void Shutdown() { Get().instance_Shutdown(); }
        
        static void SetBusVolume(BusType bus, float volume) {
            Get().GetBus(bus)->SetVolume(std::clamp(volume, 0.0f, 1.0f));
        }

        static IXAudio2Voice* GetBus(BusType bus) {
            auto& engine = Get();
            switch (bus) {
                case BusType::SFX: return engine._SFXBus;
                case BusType::Music: return engine._MusicBus;
                default: return engine._XAudioMasterVoice;
            }
        }

        static std::shared_ptr<AudioBufferResource> GetOrCreateResource(const std::filesystem::path& path);
        static void PlayOneShot(const std::filesystem::path& path, float volume = 1.0f, float pitch = 1.0f, float pan = 0.0f);

        static void UpdateOneShotSoundPool() {
            auto& pool = Get()._oneShotPool;
            std::erase_if(pool, [](const auto& sound) {
                return !sound->IsPlaying();
            });
        }

    private:
        static AudioEngine& Get() {
            static AudioEngine instance;
            return instance;
        }

        void instance_Init() {
            if (FAILED(::CoInitializeEx(nullptr, COINIT_MULTITHREADED))) return;
            ::XAudio2Create(&_XAudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
            _XAudio->CreateMasteringVoice(&_XAudioMasterVoice);

            // Create submixes
            _XAudio->CreateSubmixVoice(&_SFXBus, 2, 44100);
            _XAudio->CreateSubmixVoice(&_MusicBus, 2, 44100);
        }

        void instance_Shutdown() {
            _oneShotPool.clear();
            if (_SFXBus) _SFXBus->DestroyVoice();
            if (_MusicBus) _MusicBus->DestroyVoice();
            if (_XAudioMasterVoice) _XAudioMasterVoice->DestroyVoice();
            if (_XAudio) _XAudio->Release();
            ::CoUninitialize();
        }

        IXAudio2MasteringVoice* _XAudioMasterVoice{ nullptr };
        IXAudio2SubmixVoice* _SFXBus = { nullptr };
        IXAudio2SubmixVoice* _MusicBus = { nullptr };
        IXAudio2* _XAudio{ nullptr };

        std::unordered_map<std::string, std::shared_ptr<AudioBufferResource>> _resourceCache;
        std::vector<std::unique_ptr<Sound>> _oneShotPool;
    };

    class Sound {
    public:
        Sound() = default;
        ~Sound() { Release(); }

        void Load(const std::filesystem::path& audioFile) {
            _Resource = AudioEngine::GetOrCreateResource(audioFile);
        }

        bool IsPlaying() const { return _Playing; }

        template<FloatingPoint T = float>
        void PlaySound(T volume = 1.0f, T pitch = 1.0f, T pan = 0.0f, bool loop = false, BusType bus = BusType::SFX) {
            if (!_Resource) return;
           
            if (!_XAudioSourceVoice) {
                XAUDIO2_SEND_DESCRIPTOR sendDesc = { 0, AudioEngine::GetBus(bus) };
                XAUDIO2_VOICE_SENDS sendList = { 1, &sendDesc };
                AudioEngine::GetContext()->CreateSourceVoice(&_XAudioSourceVoice, (WAVEFORMATEX*)&_Resource->Format, 0, 2.0f, &g_VoiceCallback, &sendList);
            }

            _Playing = true;
            XAUDIO2_BUFFER xBuffer = { 0 }; 
            xBuffer.AudioBytes = (UINT32)_Resource->Data.size();
            xBuffer.pAudioData = _Resource->Data.data();
            xBuffer.Flags = XAUDIO2_END_OF_STREAM;
            xBuffer.pContext = &_Playing;
            xBuffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

            _XAudioSourceVoice->SetVolume(std::clamp(static_cast<float>(volume), 0.0f, 1.0f));
            _XAudioSourceVoice->SetFrequencyRatio(std::clamp(static_cast<float>(pitch), 0.001f, 10.0f));
            ApplyPan(static_cast<float>(pan));

            _XAudioSourceVoice->SubmitSourceBuffer(&xBuffer);
            _XAudioSourceVoice->Start(0);
        }

      
        void Release() {
            if (_XAudioSourceVoice) { _XAudioSourceVoice->DestroyVoice(); _XAudioSourceVoice = nullptr; }
            _Resource.reset();
        }

        void PlayRandomized(float baseVol = 1.0f, float basePitch = 1.0f, float variation = 0.05f) {
            static std::mt19937 gen(std::random_device{}());
            std::uniform_real_distribution<float> dist(-variation, variation);
            PlaySound(baseVol + dist(gen), basePitch + dist(gen), 0.0f);
        }

    private:
        void ApplyPan(float pan) {
            if (!_XAudioSourceVoice) return;
            XAUDIO2_VOICE_DETAILS details;
            _XAudioSourceVoice->GetVoiceDetails(&details);

            float matrix[MAX_CHANNELS_PAN];
            float angle = (std::clamp(pan, -1.0f, 1.0f) + 1.0f) * (std::numbers::pi_v<float> / 4.0f);
            float left = std::cos(angle);
            float right = std::sin(angle);

            if (details.InputChannels == 1) { // Mono to Stereo
                matrix[0] = left; matrix[1] = right;
            }
            else { // Stereo to Stereo
                matrix[0] = left; matrix[1] = 0.0f; matrix[2] = 0.0f; matrix[3] = right;
            }
            _XAudioSourceVoice->SetOutputMatrix(nullptr, details.InputChannels, 2, matrix);
        }

        bool _Playing = false;
        std::shared_ptr<AudioBufferResource> _Resource;
        IXAudio2SourceVoice* _XAudioSourceVoice{ nullptr };
    };

    inline void AudioEngine::PlayOneShot(const std::filesystem::path& path, float volume, float pitch, float pan) {
        auto& pool = Get()._oneShotPool;
        if (pool.size() >= MAX_VOICES) return;

        auto shot = std::make_unique<Sound>();
        shot->Load(path);
        shot->PlaySound(volume, pitch, pan, false);
        pool.push_back(std::move(shot));
    }

    inline std::shared_ptr<AudioBufferResource> AudioEngine::GetOrCreateResource(const std::filesystem::path& path) {
        auto& cache = Get()._resourceCache;
        std::string key = path.string();

        // not in cache
        auto resource = std::make_shared<AudioBufferResource>();
        HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (file == INVALID_HANDLE_VALUE) return nullptr;

        auto FindInFile = [&](DWORD fourcc, DWORD& size, DWORD& pos) {
            SetFilePointer(file, 0, NULL, FILE_BEGIN);
            DWORD type, dSize, riffSize = 0, fType, offset = 0;
            while (ReadFile(file, &type, 4, &dSize, NULL) && ReadFile(file, &dSize, 4, &fType, NULL)) {
                offset += 8;
                if (type == fourccRIFF) {
                    riffSize = dSize;
                    dSize = 4;
                    if (FAILED(ReadFile(file, &fType, 4, &type, NULL))) return false;
                }
                else {
                    SetFilePointer(file, dSize, NULL, FILE_CURRENT);
                }
                if (type == fourcc) {
                    size = dSize;
                    pos = offset;
                    return true;
                }
                offset += dSize;
                if (riffSize > 0 && offset >= riffSize) break;
            }
        };

        DWORD sz, pos;
        if (FindInFile(fourccFMT, sz, pos)) {
            SetFilePointer(file, pos, NULL, FILE_BEGIN);
            if (FAILED(ReadFile(file, &resource->Format, sz, &sz, NULL))) return false; 
        }
        if (FindInFile(fourccDATA, sz, pos)) {
            resource->Data.resize(sz);
            SetFilePointer(file, pos, NULL, FILE_BEGIN);
            if (FAILED(ReadFile(file, resource->Data.data(), sz, &sz, NULL))) return false;
        }
        CloseHandle(file);

        cache[key] = resource;
        return resource;
    }

} // namespace Engine::Audio