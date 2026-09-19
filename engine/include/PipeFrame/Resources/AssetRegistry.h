#ifndef PIPEFRAME_RESOURCES_ASSET_REGISTRY_H
#define PIPEFRAME_RESOURCES_ASSET_REGISTRY_H
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
namespace pipeframe {
template <typename Tag>
struct AssetHandle {
    std::uint32_t index{};
    std::uint32_t generation{};
    [[nodiscard]] explicit operator bool() const { return index != 0; }
    constexpr bool operator==(const AssetHandle&) const = default;
};
struct TextureAssetTag {};
struct FontAssetTag {};
struct ShaderAssetTag {};
struct AudioClipAssetTag {};
using TextureHandle = AssetHandle<TextureAssetTag>;
using FontHandle = AssetHandle<FontAssetTag>;
using ShaderHandle = AssetHandle<ShaderAssetTag>;
using AudioClipHandle = AssetHandle<AudioClipAssetTag>;
enum class ResourceState : std::uint8_t { Unloaded, Ready, Lost, Failed };
struct ResourceInfo {
    ResourceState state{ResourceState::Unloaded};
    std::string key;
    std::string error;
    std::uint64_t revision{};
};

template <typename Tag, typename Value>
class AssetRegistry {
public:
    using Handle = AssetHandle<Tag>;
    using Loader = std::function<std::optional<Value>(std::string&)>;
    Handle Load(std::string key, Loader loader) {
        if (auto it = keys.find(key); it != keys.end()) {
            auto& slot = slots[it->second - 1];
            if (slot.info.state == ResourceState::Ready) return {it->second, slot.generation};
            return Reload({it->second, slot.generation}, std::move(loader)) ? Handle{it->second, slot.generation}
                                                                            : Handle{it->second, slot.generation};
        }
        Slot slot;
        slot.info.key = std::move(key);
        slots.push_back(std::move(slot));
        const auto index = static_cast<std::uint32_t>(slots.size());
        keys[slots.back().info.key] = index;
        Handle handle{index, slots.back().generation};
        Reload(handle, std::move(loader));
        return handle;
    }
    bool Reload(Handle handle, Loader loader) {
        auto* slot = SlotFor(handle);
        if (!slot) return false;
        std::string error;
        auto value = loader(error);
        ++slot->info.revision;
        if (value) {
            slot->value = std::move(value);
            slot->info.state = ResourceState::Ready;
            slot->info.error.clear();
            return true;
        }
        slot->value.reset();
        slot->info.state = ResourceState::Failed;
        slot->info.error = std::move(error);
        return false;
    }
    bool MarkLost(Handle handle, std::string error = {}) {
        auto* slot = SlotFor(handle);
        if (!slot) return false;
        slot->value.reset();
        slot->info.state = ResourceState::Lost;
        slot->info.error = std::move(error);
        ++slot->info.revision;
        return true;
    }
    Value* Get(Handle handle) {
        auto* slot = SlotFor(handle);
        return slot && slot->info.state == ResourceState::Ready ? &*slot->value : nullptr;
    }
    const Value* Get(Handle handle) const {
        const auto* slot = SlotFor(handle);
        return slot && slot->info.state == ResourceState::Ready ? &*slot->value : nullptr;
    }
    const ResourceInfo* Info(Handle handle) const {
        const auto* slot = SlotFor(handle);
        return slot ? &slot->info : nullptr;
    }

private:
    struct Slot {
        std::uint32_t generation{1};
        ResourceInfo info;
        std::optional<Value> value;
    };
    Slot* SlotFor(Handle h) {
        return h.index > 0 && h.index <= slots.size() && slots[h.index - 1].generation == h.generation
                   ? &slots[h.index - 1]
                   : nullptr;
    }
    const Slot* SlotFor(Handle h) const {
        return h.index > 0 && h.index <= slots.size() && slots[h.index - 1].generation == h.generation
                   ? &slots[h.index - 1]
                   : nullptr;
    }
    std::vector<Slot> slots;
    std::unordered_map<std::string, std::uint32_t> keys;
};

struct AtlasRegion {
    TextureHandle texture{};
    float minimumU{}, minimumV{}, maximumU{1}, maximumV{1};
};
class TextureAtlas {
public:
    void Set(std::string name, AtlasRegion region) { regions[std::move(name)] = region; }
    [[nodiscard]] const AtlasRegion* Find(std::string_view name) const {
        auto it = regions.find(std::string(name));
        return it == regions.end() ? nullptr : &it->second;
    }

private:
    std::unordered_map<std::string, AtlasRegion> regions;
};
}  // namespace pipeframe
#endif
