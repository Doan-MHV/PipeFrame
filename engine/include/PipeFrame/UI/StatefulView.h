#pragma once
#include <PipeFrame/UI/ViewSource.h>

namespace pipeframe::ui {
// Shared state handle. Mounted descriptions rebuild automatically on the next UI frame.
// Builders must be pure; events mutate state through SetState. UI-thread use only.
template <class State>
class StatefulView {
public:
    using Setter = std::function<void(std::function<void(State&)>)>;
    using ReactiveBuilder = std::function<View(const State&, const Setter&)>;

private:
    struct Source final : ViewSource, std::enable_shared_from_this<Source> {
        State state;
        ReactiveBuilder builder;
        std::function<void()> onMount, onUnmount;
        View view{View::Kind::Column};
        std::uint64_t revision{1}, builtRevision{0}, generation{0};
        bool mounted{false};
        std::vector<std::shared_ptr<void>> subscriptions;
        Source(State value, ReactiveBuilder build) : state(std::move(value)), builder(std::move(build)) {}
        const View& Build() override {
            if (builtRevision != revision) {
                const auto building = revision;
                const auto lease = generation;
                Setter set = [weak = this->weak_from_this(), lease](std::function<void(State&)> mutation) {
                    if (auto source = weak.lock(); source && source->mounted && source->generation == lease) {
                        auto next = source->state;
                        mutation(next);
                        source->state = std::move(next);
                        ++source->revision;
                    }
                };
                auto next = builder(state, set);
                ValidateView(next);
                view = std::move(next);
                builtRevision = building;
            }
            return view;
        }
        std::uint64_t Revision() const override { return revision; }
        void Mount() override {
            if (mounted) throw std::logic_error("A stateful source is already mounted");
            mounted = true;
            try {
                if (onMount) onMount();
            } catch (...) {
                mounted = false;
                subscriptions.clear();
                throw;
            }
        }
        void Unmount() noexcept override {
            if (!mounted) return;
            mounted = false;
            ++generation;
            subscriptions.clear();
            // Release captures from the rendered description on removal.
            view = View{View::Kind::Column};
            builtRevision = 0;
            if (onUnmount) {
                try {
                    onUnmount();
                } catch (...) { /* destruction must complete */
                }
            }
        }
    };

public:
    using Builder = std::function<View(const State&)>;
    StatefulView(State initial, Builder builder)
        : StatefulView(std::move(initial),
                       ReactiveBuilder([builder = std::move(builder)](const State& state, const Setter&) {
                           if (!builder) throw std::invalid_argument("A stateful view needs a builder");
                           return builder(state);
                       })) {}
    StatefulView(State initial, ReactiveBuilder builder)
        : source(std::make_shared<Source>(std::move(initial), std::move(builder))) {
        if (!source->builder) throw std::invalid_argument("A stateful view needs a builder");
    }
    const State& GetState() const { return source->state; }
    template <class Mutation>
    void SetState(Mutation mutation) {
        auto next = source->state;
        mutation(next);
        source->state = std::move(next);
        ++source->revision;
    }
    const View& Build() { return source->Build(); }
    template <class Handle>
    void RetainForMount(Handle&& handle) {
        if (!IsMounted()) throw std::logic_error("Subscriptions require a mounted view");
        source->subscriptions.push_back(std::make_shared<std::decay_t<Handle>>(std::forward<Handle>(handle)));
    }
    View Describe(std::string key) const { return views::Stateful(std::move(key), source); }
    bool IsMounted() const { return source->mounted; }
    void SetLifecycle(std::function<void()> mount, std::function<void()> unmount) {
        if (IsMounted()) throw std::logic_error("Configure lifecycle before mounting");
        source->onMount = std::move(mount);
        source->onUnmount = std::move(unmount);
    }

private:
    std::shared_ptr<Source> source;
};
}  // namespace pipeframe::ui
