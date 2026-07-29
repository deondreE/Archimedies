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

    // --- Callbacks ---
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

    inline SoundCallback g_VoiceCallback;

    // --- Forward Declarations ---
    class Sound;

    // --- AudioEngine Definition ---
    class AudioEngine {
    public:
        static constexpr size_t MAX_VOICES = 32;

        static void Init() { Get().instance_Init(); }
        static IXAudio2* GetContext() { return Get()._XAudio; }
        static void Shutdown() { Get().instance_Shutdown(); }

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
        }

        void instance_Shutdown() {
            _oneShotPool.clear();
            if (_XAudioMasterVoice) _XAudioMasterVoice->DestroyVoice();
            if (_XAudio) _XAudio->Release();
            ::CoUninitialize();
        }

        IXAudio2MasteringVoice* _XAudioMasterVoice{ nullptr };
        IXAudio2* _XAudio{ nullptr };
        std::vector<std::unique_ptr<Sound>> _oneShotPool;
    };

    class Sound {
    public:
        Sound() = default;
        ~Sound() { Release(); }

        void Load(const std::filesystem::path& audioFile) {
            HANDLE File = CreateFileW(audioFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
            if (INVALID_HANDLE_VALUE == File) return;

            // RIFF
            DWORD ChunkSize, ChunkPosition;
            FindChunk(File, fourccRIFF, ChunkSize, ChunkPosition);
            
            // TYPE
            DWORD FileType;
            ReadChunkData(File, &FileType, sizeof(DWORD), ChunkPosition);
            if (FileType != fourccWAVE) { CloseHandle(File); return; }

            // FMT
            FindChunk(File, fourccFMT, ChunkSize, ChunkPosition);
            ReadChunkData(File, &_Wfx, ChunkSize, ChunkPosition);

            // DATA
            FindChunk(File, fourccDATA, ChunkSize, ChunkPosition);
            BYTE* DataBuffer = new BYTE[ChunkSize];
            ReadChunkData(File, DataBuffer, ChunkSize, ChunkPosition);

            _Buffer.AudioBytes = ChunkSize;
            _Buffer.pAudioData = DataBuffer;
            _Buffer.Flags = XAUDIO2_END_OF_STREAM;
            _Buffer.pContext = &_Playing;

            CloseHandle(File);
        }

        bool IsPlaying() const { return _Playing; }

        template<FloatingPoint T = float>
        void PlaySound(T volume = 1.0f, T pitch = 1.0f, T pan = 0.0f, bool loop = false) {
            IXAudio2* ctx = AudioEngine::GetContext();
            if (!ctx || !_Buffer.pAudioData) return;

            if (!_XAudioSourceVoice) {
                ctx->CreateSourceVoice(&_XAudioSourceVoice, (WAVEFORMATEX*)&_Wfx, 0, 2.0f, &g_VoiceCallback);
            }

            _Playing = true;
            _Buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

            SetVolume(static_cast<float>(volume));
            SetPitch(static_cast<float>(pitch));
            ApplyPan(static_cast<float>(pan));

            _XAudioSourceVoice->SubmitSourceBuffer(&_Buffer);
            _XAudioSourceVoice->Start(0);
        }

        void Stop() { if (_XAudioSourceVoice) _XAudioSourceVoice->Stop(0); }

        void Release() {
            if (_XAudioSourceVoice) { _XAudioSourceVoice->DestroyVoice(); _XAudioSourceVoice = nullptr; }
            if (_Buffer.pAudioData) { delete[](BYTE*)_Buffer.pAudioData; _Buffer.pAudioData = nullptr; }
        }

        void PlayRandomized(float baseVol = 1.0f, float basePitch = 1.0f, float variation = 0.05f) {
            static std::mt19937 gen(std::random_device{}());
            std::uniform_real_distribution<float> dist(-variation, variation);
            PlaySound(baseVol + dist(gen), basePitch + dist(gen), 0.0f);
        }

        void SetVolume(float volume) {
            if (_XAudioSourceVoice) _XAudioSourceVoice->SetVolume(std::clamp(volume, 0.0f, 1.0f));
        }

        void SetPitch(float ratio) {
            if (_XAudioSourceVoice) _XAudioSourceVoice->SetFrequencyRatio(std::clamp(ratio, 0.0001f, 10.0f));
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

        HRESULT FindChunk(HANDLE File, DWORD fourcc, DWORD& ChunkSize, DWORD& ChunkDataPosition) {
            if (INVALID_SET_FILE_POINTER == SetFilePointer(File, 0, NULL, FILE_BEGIN)) return HRESULT_FROM_WIN32(GetLastError());
            DWORD ChunkType, ChunkDataSize, RIFFDataSize = 0, FileType, Offset = 0;
            while (true) {
                DWORD Read;
                if (0 == ReadFile(File, &ChunkType, sizeof(DWORD), &Read, NULL)) return HRESULT_FROM_WIN32(GetLastError());
                if (0 == ReadFile(File, &ChunkDataSize, sizeof(DWORD), &Read, NULL)) return HRESULT_FROM_WIN32(GetLastError());
                Offset += sizeof(DWORD) * 2;
                if (ChunkType == fourccRIFF) {
                    RIFFDataSize = ChunkDataSize; ChunkDataSize = 4;
                    ReadFile(File, &FileType, sizeof(DWORD), &Read, NULL);
                }
                else {
                    SetFilePointer(File, ChunkDataSize, NULL, FILE_CURRENT);
                }
                if (ChunkType == fourcc) { ChunkSize = ChunkDataSize; ChunkDataPosition = Offset; return S_OK; }
                Offset += ChunkDataSize;
                if (RIFFDataSize > 0 && Offset >= RIFFDataSize) return S_FALSE;
            }
        }

        HRESULT ReadChunkData(HANDLE File, void* buffer, DWORD bufferSize, DWORD bufferOffset) {
            SetFilePointer(File, bufferOffset, NULL, FILE_BEGIN);
            DWORD dwRead;
            return ReadFile(File, buffer, bufferSize, &dwRead, NULL) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
        }

        WAVEFORMATEXTENSIBLE _Wfx = { 0 };
        XAUDIO2_BUFFER _Buffer = { 0 };
        bool _Playing = false;
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

} // namespace Engine::Audio