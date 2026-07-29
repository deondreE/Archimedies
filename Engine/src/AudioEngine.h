#pragma once
#include "archpch.h"
#include <concepts>
#include <filesystem>
#include <numbers>
#include <xaudio2.h>
#include <xapo.h>      

namespace Engine::Audio {

    // @Todo: 3d Audio is the only thing left i'm pretty sure. X3DAudio
    // @Todo: There is no UI around audio at all.
    // @Todo: We want to support more then .WAV

    template<typename T>
    concept FloatingPoint = std::is_floating_point_v<T>;

    // Use XAudio2 built-in macros for FOURCC to avoid cast warnings
#ifndef fourccRIFF
#define fourccRIFF (DWORD)'FFIR'
#define fourccDATA (DWORD)'atad'
#define fourccFMT  (DWORD)' tmf'
#define fourccWAVE (DWORD)'EVAW'
#endif

#define MAX_CHANNELS_PAN 16

    enum class BusType { Master, SFX, Music };

    struct AudioBufferResource {
        WAVEFORMATEXTENSIBLE Format = { 0 };
        std::vector<BYTE> Data;
    };

    class SoundCallback : public IXAudio2VoiceCallback {
    public:
        void OnStreamEnd() override {}
        void OnVoiceProcessingPassStart(UINT32) override {}
        void OnVoiceProcessingPassEnd() override {}
        void OnVoiceError(void*, HRESULT) override {}
        void OnBufferStart(void*) override {}
        void OnBufferEnd(void* pContext) override {
            if (pContext) *static_cast<bool*>(pContext) = false;
        }
        void OnLoopEnd(void*) override {}
    };

    inline SoundCallback g_VoiceCallback;

    class Sound;

    class AudioEngine {
    public:
        // Max Number of voices playing concurrently.
        static constexpr size_t MAX_VOICES = 32;

        static void Init() { Get().instance_Init(); }
        static void Shutdown() { Get().instance_Shutdown(); }
        static IXAudio2* GetContext() { return Get()._XAudio; }

        static void SetBusVolume(BusType bus, float volume) {
            GetBusVoice(bus)->SetVolume(std::clamp(volume, 0.0f, 1.0f));
        }

        static float GetBusVolume(BusType bus) {
            float vol = 0.0f;
            GetBusVoice(bus)->GetVolume(&vol);
            return vol;
        }

        static IXAudio2Voice* GetBusVoice(BusType bus) {
            auto& engine = Get();
            if (bus == BusType::SFX && engine._SFXBus) return engine._SFXBus;
            if (bus == BusType::Music && engine._MusicBus) return engine._MusicBus;
            return engine._XAudioMasterVoice;
        }

        static std::unordered_map<std::string, std::shared_ptr<AudioBufferResource>> GetCache() {
            auto& engine = Get();
            return engine._resourceCache;
        }

        // Ensures that loading a file will only happen once.
        static std::shared_ptr<AudioBufferResource> GetOrCreateResource(const std::filesystem::path& path);

        // A Sound that is loaded then unloaded from the active context. 
        static void PlayOneShot(const std::filesystem::path& path, float volume = 1.0f, float pitch = 1.0f, float pan = 0.0f);

        static void ClearCache() { Get()._resourceCache.clear(); }
        static size_t GetActiveVoiceCount() { return Get()._oneShotPool.size(); }

        static void UpdateOneShotSoundPool() {
            std::erase_if(Get()._oneShotPool, [](const auto& sound) {
                return !sound->IsPlaying();
            });
        }

    private:
        static AudioEngine& Get() { static AudioEngine instance; return instance; }

        void instance_Init() {
            if (FAILED(::CoInitializeEx(nullptr, COINIT_MULTITHREADED))) return;
            if (FAILED(::XAudio2Create(&_XAudio, 0, XAUDIO2_DEFAULT_PROCESSOR))) return;
            if (FAILED(_XAudio->CreateMasteringVoice(&_XAudioMasterVoice))) return;
            _XAudio->CreateSubmixVoice(&_SFXBus, 2, 44100);

            _XAudio->CreateSubmixVoice(&_MusicBus, 2, 44100);
        }

        void instance_Shutdown() {
            _oneShotPool.clear();
            _resourceCache.clear();
            if (_SFXBus) { _SFXBus->DestroyVoice(); _SFXBus = nullptr; }
            if (_MusicBus) { _MusicBus->DestroyVoice(); _MusicBus = nullptr; }
            if (_XAudioMasterVoice) { _XAudioMasterVoice->DestroyVoice(); _XAudioMasterVoice = nullptr; }
            if (_XAudio) { _XAudio->Release(); _XAudio = nullptr; }
            ::CoUninitialize();
        }

        IXAudio2MasteringVoice* _XAudioMasterVoice{ nullptr };
        IXAudio2SubmixVoice* _SFXBus{ nullptr };
        IXAudio2SubmixVoice* _MusicBus{ nullptr };
        IXAudio2* _XAudio{ nullptr };

        std::unordered_map<std::string, std::shared_ptr<AudioBufferResource>> _resourceCache;
        std::vector<std::unique_ptr<Sound>> _oneShotPool;
    };

    /* Sound is more like a SoundPlayer, but for now we will keep this naming.*/
    class Sound {
    public:
        Sound() = default;
        ~Sound() { Release(); }

        void Load(const std::filesystem::path& audioFile) {
            _Resource = AudioEngine::GetOrCreateResource(audioFile);
        }

        template<FloatingPoint T = float>
        void PlaySound(T volume = 1.0f, T pitch = 1.0f, T pan = 0.0f, bool loop = false, BusType bus = BusType::SFX) {
            if (!_Resource) return;

            if (!_XAudioSourceVoice) {
                XAUDIO2_SEND_DESCRIPTOR sendDesc = { 0, AudioEngine::GetBusVoice(bus) };
                XAUDIO2_VOICE_SENDS sendList = { 1, &sendDesc };
                if (FAILED(AudioEngine::GetContext()->CreateSourceVoice(&_XAudioSourceVoice, (WAVEFORMATEX*)&_Resource->Format, 0, 2.0f, &g_VoiceCallback, &sendList)))
                {
                    LOG_ERROR("CreateSourceVoice: Failed");
                    return;
                }
            }

            _Playing = true;

            XAUDIO2_BUFFER xBuffer = { 0 };
            xBuffer.AudioBytes = static_cast<UINT32>(_Resource->Data.size());
            xBuffer.pAudioData = _Resource->Data.data();
            xBuffer.Flags = XAUDIO2_END_OF_STREAM;
            xBuffer.pContext = &_Playing;
            xBuffer.LoopCount = _Loop ? XAUDIO2_LOOP_INFINITE : 0;

            SetVolume(static_cast<float>(volume));
            SetPitch(static_cast<float>(pitch));
            SetLooping(loop);
            SetPan(static_cast<float>(pan));

            if (SUCCEEDED(_XAudioSourceVoice->SubmitSourceBuffer(&xBuffer))) {
                _XAudioSourceVoice->Start(0);
            }
        }

        void Stop() {
            if (_XAudioSourceVoice) {
                _XAudioSourceVoice->Stop(0);
                _XAudioSourceVoice->FlushSourceBuffers();
                _Playing = false;
            }
        }

        void Pause() { if (_XAudioSourceVoice) _XAudioSourceVoice->Stop(0); }
        void Resume() { if (_XAudioSourceVoice) _XAudioSourceVoice->Start(0); }

        void Release() {
            Stop();
            if (_XAudioSourceVoice) { _XAudioSourceVoice->DestroyVoice(); _XAudioSourceVoice = nullptr; }
            _Resource.reset();
        }

        void PlayRandomized(float baseVol = 1.0f, float basePitch = 1.0f, float variation = 0.05f) {
            static std::mt19937 gen(std::random_device{}());
            std::uniform_real_distribution<float> dist(-variation, variation);
            PlaySound(baseVol + dist(gen), basePitch + dist(gen), 0.0f);
        }

        void SetVolume(float volume) {
            _Volume = std::clamp(volume, 0.0f, 1.0f);
            if (_XAudioSourceVoice) _XAudioSourceVoice->SetVolume(_Volume);
        }

        void SetPitch(float pitch) {
            _Pitch = std::clamp(pitch, 0.0001f, 10.0f);
            if (_XAudioSourceVoice) _XAudioSourceVoice->SetFrequencyRatio(_Pitch);
        }

        void SetPitchSemitones(float semitones) { SetPitch(std::pow(2.0f, semitones / 12.0f)); }

        void SetPan(float pan) {
            _Pan = std::clamp(pan, -1.0f, 1.0f);
            ApplyPanMatrix();
        }

        void SetLooping(bool loop) { _Loop = loop; }

        [[nodiscard]] bool IsPlaying() const { return _Playing; }
        [[nodiscard]] float GetVolume() const { return _Volume; }
        [[nodiscard]] float GetPitch() const { return _Pitch; }
        [[nodiscard]] float GetPan() const { return _Pan; }
        [[nodiscard]] bool IsLooping() const { return _Loop; }

    private:
        // @See: https://learn.microsoft.com/en-us/windows/win32/xaudio2/how-to--pan-a-sound
        // This works not completly sure how.
        void ApplyPanMatrix() {
            if (!_XAudioSourceVoice) return;
            XAUDIO2_VOICE_DETAILS details;
            _XAudioSourceVoice->GetVoiceDetails(&details);

            float matrix[MAX_CHANNELS_PAN];
            float angle = (_Pan + 1.0f) * (std::numbers::pi_v<float> / 4.0f);
            float left = std::cos(angle);
            float right = std::sin(angle);

            if (details.InputChannels == 1) {
                matrix[0] = left; matrix[1] = right;
            }
            else {
                matrix[0] = left; matrix[1] = 0.0f; matrix[2] = 0.0f; matrix[3] = right;
            }
            _XAudioSourceVoice->SetOutputMatrix(nullptr, details.InputChannels, 2, matrix);
        }

        std::shared_ptr<AudioBufferResource> _Resource;
        IXAudio2SourceVoice* _XAudioSourceVoice{ nullptr };
        bool _Loop = false;
        bool _Playing = false;
        float _Volume = 1.0f;
        float _Pitch = 1.0f;
        float _Pan = 0.0f;
    };

    inline void AudioEngine::PlayOneShot(const std::filesystem::path& path, float volume, float pitch, float pan) {
        if (Get()._oneShotPool.size() >= MAX_VOICES) return;
        auto shot = std::make_unique<Sound>();
        shot->Load(path);
        shot->PlaySound(volume, pitch, pan, false);
        Get()._oneShotPool.push_back(std::move(shot));
    }

    inline std::shared_ptr<AudioBufferResource> AudioEngine::GetOrCreateResource(const std::filesystem::path& path) {
        auto& cache = Get()._resourceCache;
        std::string key = path.string();

        if (cache.contains(key)) return cache[key];

        auto resource = std::make_shared<AudioBufferResource>();
        HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (file == INVALID_HANDLE_VALUE) return nullptr;

        auto FindInFile = [&](DWORD fourcc, DWORD& size, DWORD& pos) -> bool {
            if (INVALID_SET_FILE_POINTER == SetFilePointer(file, 0, NULL, FILE_BEGIN)) return false;
            DWORD type, dSize, riffSize = 0, fType, offset = 0;
            DWORD bytesRead = 0;
            while (ReadFile(file, &type, 4, &bytesRead, NULL) && ReadFile(file, &dSize, 4, &bytesRead, NULL)) {
                offset += 8;
                if (type == fourccRIFF) {
                    riffSize = dSize; dSize = 4;
                    if (!ReadFile(file, &fType, 4, &bytesRead, NULL)) return false;
                }
                else {
                    if (INVALID_SET_FILE_POINTER == SetFilePointer(file, dSize, NULL, FILE_CURRENT)) return false;
                }
                if (type == fourcc) {
                    size = dSize; pos = offset; return true;
                }
                offset += dSize;
                if (riffSize > 0 && offset >= riffSize) break;
            }
            return false;
        };

        DWORD sz = 0, pos = 0;
        DWORD read = 0;
        if (FindInFile(fourccFMT, sz, pos)) {
            SetFilePointer(file, pos, NULL, FILE_BEGIN);
            if (!ReadFile(file, &resource->Format, sz, &read, NULL)) { CloseHandle(file); return nullptr; }
        }
        if (FindInFile(fourccDATA, sz, pos)) {
            resource->Data.resize(sz);
            SetFilePointer(file, pos, NULL, FILE_BEGIN);
            if (!ReadFile(file, resource->Data.data(), sz, &read, NULL)) { CloseHandle(file); return nullptr; }
        }
        CloseHandle(file);

        return cache[key] = resource;
    }

} // namespace Engine::Audio