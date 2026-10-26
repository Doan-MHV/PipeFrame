#pragma once
#include <PipeFrame/Core/Signal.h>
#include <PipeFrame/Events/Event.h>
#include <any>
#include <concepts>
#include <memory>
#include <typeindex>
#include <unordered_map>

namespace pipeframe {
// Store this handle as a member. Destruction stops listening, including when
// the channel owner has already been destroyed. Handles cannot be copied.
class EventSubscription final {
public:
    EventSubscription() = default;
    explicit EventSubscription(std::function<void()> disconnect) : disconnect(std::move(disconnect)) {}
    ~EventSubscription() { Disconnect(); }
    EventSubscription(const EventSubscription&) = delete;
    EventSubscription& operator=(const EventSubscription&) = delete;
    EventSubscription(EventSubscription&& other) noexcept : disconnect(std::exchange(other.disconnect, {})) {}
    EventSubscription& operator=(EventSubscription&& other) noexcept {
        if (this != &other) { Disconnect(); disconnect = std::exchange(other.disconnect, {}); }
        return *this;
    }
    void Disconnect() { if (auto action = std::exchange(disconnect, {})) action(); }
private:
    std::function<void()> disconnect;
};

// Synchronous simulation-thread delivery. Events are not queued or retained.
class EventChannels {
public:
    EventChannels() = default;
    virtual ~EventChannels() = default;
    EventChannels(const EventChannels&) = delete;
    EventChannels& operator=(const EventChannels&) = delete;
    template<std::derived_from<Event> T, class Callback>
    [[nodiscard]] EventSubscription Listen(Callback&& callback) {
        auto channel = Channel<T>();
        const auto token = channel->Connect(std::forward<Callback>(callback));
        return EventSubscription([weak = std::weak_ptr(channel), token] {
            if (auto live = weak.lock()) live->Disconnect(token);
        });
    }
    template<std::derived_from<Event> T>
    void Publish(const T& event) {
        auto channel = Channel<T>();
        channel->Emit(event);
    }
private:
    template<std::derived_from<Event> T>
    std::shared_ptr<Signal<const T&>> Channel() {
        auto [it, inserted] = channels.try_emplace(typeid(T));
        if (inserted) it->second = std::make_shared<Signal<const T&>>();
        return std::any_cast<std::shared_ptr<Signal<const T&>>>(it->second);
    }
    std::unordered_map<std::type_index, std::any> channels;
};
}
