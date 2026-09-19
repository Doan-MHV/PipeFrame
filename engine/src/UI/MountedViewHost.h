#pragma once
#include "ViewRenderer.h"
#include <PipeFrame/Backend/SFML/UI/OverlayPanel.h>
#include <PipeFrame/UI/MountedView.h>
#include <PipeFrame/UI/ViewSource.h>
#include <map>
#include <set>

namespace pipeframe::ui {
class MountedViewHost final : public OverlayPanel {
    using Path = std::vector<std::string>;
    struct Entry {
        std::shared_ptr<ViewSource> source;
        std::uint64_t revision;
    };
    std::shared_ptr<detail::ViewMountState> state;
    std::map<Path, Entry> mounted;
    const sf::Font &font;
    UITheme theme;
    std::uint64_t renderedRevision{};
    bool disposed{false};

    View Resolve(const View &view, Path path, std::map<Path, Entry> &next, std::set<ViewSource *> &seen,
                 unsigned depth = 0) {
        if (depth > 128)
            throw std::invalid_argument("View nesting exceeds 128 levels");
        path.push_back(view.key);
        path.push_back(std::to_string(static_cast<int>(view.kind)));
        if (view.kind == View::Kind::Stateful) {
            if (!seen.insert(view.source.get()).second)
                throw std::invalid_argument("A stateful source cannot occupy two locations");
            const auto revision = view.source->Revision();
            const auto &built = view.source->Build();
            ValidateView(built);
            next.emplace(path, Entry{view.source, revision});
            auto wrapper = views::Column(view.key, {Resolve(built, path, next, seen, depth + 1)});
            wrapper.instanceIdentity = view.source.get();
            wrapper.spacing = 0;
            wrapper.enabled = view.enabled;
            wrapper.flex = view.flex;
            wrapper.width = view.width;
            if (view.stretchHeight)
                wrapper.FillHeight();
            return wrapper;
        }
        View result = view;
        result.children.clear();
        for (const auto &child : view.children)
            result.children.push_back(Resolve(child, path, next, seen, depth + 1));
        return result;
    }
    void Dispose() noexcept {
        if (disposed)
            return;
        // Child-first lifecycle cleanup.
        for (auto it = mounted.rbegin(); it != mounted.rend(); ++it)
            it->second.source->Unmount();
        mounted.clear();
        state->active = false;
        state->view = View{View::Kind::Column};
        disposed = true;
    }

  protected:
    void OnUpdate(float) override {
        if (!state->active) {
            Dispose();
            return;
        }
        const auto requestedRevision = state->revision;
        bool dirty = renderedRevision != requestedRevision;
        for (const auto &[path, entry] : mounted)
            dirty |= entry.source->Revision() != entry.revision;
        if (!dirty) {
            // Geometry changes arrange through OverlayPanel; clean frames need no
            // description copy, validation, reconciliation or explicit relayout.
            Arrange({{state->x, state->y}, {state->width, state->height}});
            return;
        }
        ValidateView(state->view);
        std::map<Path, Entry> next;
        std::set<ViewSource *> seen;
        auto resolved = Resolve(state->view, {}, next, seen);
        bool changed = renderedRevision != state->revision || next.size() != mounted.size();
        for (const auto &[path, entry] : next) {
            const auto old = mounted.find(path);
            changed |=
                old == mounted.end() || old->second.source != entry.source || old->second.revision != entry.revision;
        }
        if (changed) {
            // Build/validate before changing live widgets. Sources may not move between keys in one frame.
            for (const auto &[path, entry] : next) {
                const auto old = mounted.find(path);
                if (old != mounted.end() && old->second.source == entry.source)
                    continue;
                for (const auto &[oldPath, oldEntry] : mounted)
                    if (oldEntry.source == entry.source)
                        throw std::invalid_argument("Remove a stateful source before mounting it at another key");
            }
            std::vector<std::shared_ptr<ViewSource>> added;
            try {
                for (const auto &[path, entry] : next) {
                    const auto old = mounted.find(path);
                    if (old == mounted.end() || old->second.source != entry.source) {
                        entry.source->Mount();
                        added.push_back(entry.source);
                    }
                }
                BeginCompositionPass(true);
                ViewRenderer(font, theme).Render(*this, resolved);
                EndCompositionPass();
            } catch (...) {
                for (auto it = added.rbegin(); it != added.rend(); ++it)
                    (*it)->Unmount();
                throw;
            }
            for (auto it = mounted.rbegin(); it != mounted.rend(); ++it) {
                const auto found = next.find(it->first);
                if (found == next.end() || found->second.source != it->second.source)
                    it->second.source->Unmount();
            }
            mounted = std::move(next);
            renderedRevision = requestedRevision;
        }
        Arrange({{state->x, state->y}, {state->width, state->height}});
        RefreshLayout();
    }
    bool ClipsChildren() const override { return true; }

  public:
    MountedViewHost(std::shared_ptr<detail::ViewMountState> state, const sf::Font &font, const UITheme &theme)
        : state(std::move(state)), font(font), theme(theme) {
        SetFillColor(sf::Color::Transparent);
        SetOutlineThickness(0);
        SetHitTestVisible(false);
    }
    ~MountedViewHost() override { Dispose(); }
    bool IsDisposed() const override { return disposed; }
};
} // namespace pipeframe::ui
