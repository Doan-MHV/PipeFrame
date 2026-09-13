#include <PipeFrame/Audio/AudioService.h>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <algorithm>
#include <unordered_map>
#include <vector>
namespace pipeframe {
struct AudioService::Impl{struct Clip{std::string path;sf::SoundBuffer buffer;ResourceState state{ResourceState::Unloaded};};std::unordered_map<std::uint32_t,Clip> clips;std::unordered_map<std::string,std::uint32_t> clipIds;std::vector<std::unique_ptr<sf::Sound>> voices;std::uint32_t next{1};float master{1};};
AudioService::AudioService():impl(std::make_unique<Impl>()){}AudioService::~AudioService()=default;AudioService::AudioService(AudioService&&)noexcept=default;AudioService&AudioService::operator=(AudioService&&)noexcept=default;
AudioClipHandle AudioService::LoadClip(const std::string &path,std::string *error){if(const auto cached=impl->clipIds.find(path);cached!=impl->clipIds.end())return {cached->second,1};const auto id=impl->next++;Impl::Clip clip;clip.path=path;if(clip.buffer.loadFromFile(path))clip.state=ResourceState::Ready;else{clip.state=ResourceState::Failed;if(error)*error="Unable to load audio clip: "+path;}impl->clips.emplace(id,std::move(clip));impl->clipIds.emplace(path,id);return {id,1};}
bool AudioService::ReloadClip(AudioClipHandle handle,std::string *error){auto it=impl->clips.find(handle.index);if(it==impl->clips.end()||handle.generation!=1)return false;if(it->second.buffer.loadFromFile(it->second.path)){it->second.state=ResourceState::Ready;return true;}it->second.state=ResourceState::Failed;if(error)*error="Unable to reload audio clip: "+it->second.path;return false;}
bool AudioService::Play(AudioClipHandle handle,float volume,float pitch){auto it=impl->clips.find(handle.index);if(it==impl->clips.end()||handle.generation!=1||it->second.state!=ResourceState::Ready)return false;std::erase_if(impl->voices,[](const auto &voice){return voice->getStatus()==sf::SoundSource::Status::Stopped;});auto voice=std::make_unique<sf::Sound>(it->second.buffer);voice->setVolume(std::clamp(volume*impl->master,0.0f,1.0f)*100.0f);voice->setPitch(std::max(0.01f,pitch));voice->play();impl->voices.push_back(std::move(voice));return true;}
void AudioService::StopAll(){for(auto &voice:impl->voices)voice->stop();impl->voices.clear();}void AudioService::SetMasterVolume(float volume){impl->master=std::clamp(volume,0.0f,1.0f);}ResourceState AudioService::State(AudioClipHandle handle)const{auto it=impl->clips.find(handle.index);return it==impl->clips.end()||handle.generation!=1?ResourceState::Unloaded:it->second.state;}
} // namespace pipeframe
