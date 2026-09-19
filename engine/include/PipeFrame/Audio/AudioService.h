#ifndef PIPEFRAME_AUDIO_AUDIO_SERVICE_H
#define PIPEFRAME_AUDIO_AUDIO_SERVICE_H

#include <PipeFrame/Resources/AssetRegistry.h>

#include <memory>
#include <string>

namespace pipeframe {

class AudioService {
public:
    AudioService();
    ~AudioService();
    AudioService(AudioService&&) noexcept;
    AudioService& operator=(AudioService&&) noexcept;
    AudioService(const AudioService&) = delete;
    AudioService& operator=(const AudioService&) = delete;
    AudioClipHandle LoadClip(const std::string& path, std::string* error = nullptr);
    bool ReloadClip(AudioClipHandle handle, std::string* error = nullptr);
    bool Play(AudioClipHandle handle, float volume = 1.0f, float pitch = 1.0f);
    void StopAll();
    void SetMasterVolume(float volume);
    [[nodiscard]] ResourceState State(AudioClipHandle handle) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

}  // namespace pipeframe
#endif
