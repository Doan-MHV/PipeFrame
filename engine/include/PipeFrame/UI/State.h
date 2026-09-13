#ifndef PIPEFRAME_UI_STATE_H
#define PIPEFRAME_UI_STATE_H

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace pipeframe::ui {

enum class StateLifetime {
    Application,
    Visual,
};

template <typename Value> class State {
  private:
    struct Core {
        Value value;
        std::size_t nextObserverId = 1;
        std::map<std::size_t, std::function<void(const Value &)>> observers;
    };

  public:
    class Subscription {
      public:
        Subscription() = default;
        Subscription(const Subscription &) = delete;
        Subscription &operator=(const Subscription &) = delete;

        Subscription(Subscription &&other) noexcept
            : core(std::move(other.core)), observerId(std::exchange(other.observerId, 0)) {}

        Subscription &operator=(Subscription &&other) noexcept {
            if (this != &other) {
                Reset();
                core = std::move(other.core);
                observerId = std::exchange(other.observerId, 0);
            }
            return *this;
        }

        ~Subscription() { Reset(); }

        void Reset() {
            if (observerId != 0) {
                if (const std::shared_ptr<Core> locked = core.lock()) {
                    locked->observers.erase(observerId);
                }
            }
            observerId = 0;
            core.reset();
        }

      private:
        friend class State;
        Subscription(const std::shared_ptr<Core> &newCore, const std::size_t newObserverId)
            : core(newCore), observerId(newObserverId) {}

        std::weak_ptr<Core> core;
        std::size_t observerId = 0;
    };

    explicit State(Value initialValue, const StateLifetime newLifetime = StateLifetime::Visual)
        : core(std::make_shared<Core>(Core{std::move(initialValue)})), lifetime(newLifetime) {}

    const Value &Get() const { return core->value; }

    StateLifetime GetLifetime() const { return lifetime; }

    void Set(Value newValue) {
        const std::shared_ptr<Core> activeCore = core;
        if constexpr (requires { activeCore->value == newValue; }) {
            if (activeCore->value == newValue) {
                return;
            }
        }
        activeCore->value = std::move(newValue);
        const Value publishedValue = activeCore->value;

        std::vector<std::function<void(const Value &)>> callbacks;
        callbacks.reserve(activeCore->observers.size());
        for (const auto &[id, observer] : activeCore->observers) {
            (void)id;
            callbacks.push_back(observer);
        }
        for (const auto &callback : callbacks) {
            callback(publishedValue);
        }
    }

    template <typename Observer> Subscription Observe(Observer &&observer) {
        const std::size_t observerId = core->nextObserverId++;
        core->observers.emplace(observerId, std::forward<Observer>(observer));
        return Subscription{core, observerId};
    }

    std::size_t GetObserverCount() const { return core->observers.size(); }

  private:
    std::shared_ptr<Core> core;
    StateLifetime lifetime;
};

template <typename WidgetType, typename Value, typename Setter>
void BindProperty(WidgetType &widget, State<Value> &state, Setter &&setter) {
    using SetterType = std::decay_t<Setter>;
    SetterType retainedSetter = std::forward<Setter>(setter);
    std::invoke(retainedSetter, widget, state.Get());

    auto subscription = state.Observe(
        [&widget, lifetime=widget.Lifetime(), retainedSetter = std::move(retainedSetter)](const Value &value) mutable {
            if (lifetime.expired()) return;
            std::invoke(retainedSetter, widget, value);
        });
    widget.RetainBinding(std::move(subscription));
}

template <typename WidgetType> void BindVisible(WidgetType &widget, State<bool> &state) {
    BindProperty(widget, state,
                 [](WidgetType &target, const bool visible) { target.SetVisible(visible); });
}

template <typename WidgetType> void BindEnabled(WidgetType &widget, State<bool> &state) {
    BindProperty(widget, state,
                 [](WidgetType &target, const bool enabled) { target.SetEnabled(enabled); });
}

template <typename WidgetType, typename Value, typename Setter>
void BindSelection(WidgetType &widget, State<Value> &state, Setter &&setter) {
    BindProperty(widget, state, std::forward<Setter>(setter));
}

} // namespace pipeframe::ui

#endif
