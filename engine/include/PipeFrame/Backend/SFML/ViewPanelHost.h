#pragma once
#include <PipeFrame/Backend/SFML/UI/Panel.h>
#include <PipeFrame/Backend/SFML/UI/UITheme.h>
#include <PipeFrame/UI/MountedView.h>
#include <PipeFrame/UI/StatefulView.h>

#include <SFML/Graphics/Font.hpp>

namespace pipeframe::ui {
// Native embedding boundary for declarative UI in a dock/window. Application
// subclasses describe views; the engine owns reconciliation, layout and input.
class NativeViewPanel : public Panel {
public:
    explicit NativeViewPanel(const sf::Font& font, const UITheme& theme = UITheme::Dark());
    ~NativeViewPanel() override;
    void InvalidateView();

protected:
    virtual View BuildNativeView() = 0;
    void OnUpdate(float seconds) override;
    void OnGeometryChanged() override;

private:
    StatefulView<std::uint64_t> revision;
    std::shared_ptr<detail::ViewMountState> state;
    MountedView mount;
    bool dirty{true};
};
}  // namespace pipeframe::ui

namespace pipeframe::ui {
class ViewBuilderPanel final : public NativeViewPanel {
public:
    ViewBuilderPanel(const sf::Font& font, std::function<View()> builder, const UITheme& theme = UITheme::Dark())
        : NativeViewPanel(font, theme), builder(std::move(builder)) {}

protected:
    View BuildNativeView() override { return builder(); }

private:
    std::function<View()> builder;
};
}  // namespace pipeframe::ui

#include <PipeFrame/UI/ViewPanel.h>
namespace pipeframe::backend::sfml {
// Native container for a backend-neutral presenter. Native base is destroyed first,
// unmounting callbacks before the presenter state is destroyed.
template <typename Presenter>
class HostedViewPanel final : public Presenter, public pipeframe::ui::NativeViewPanel {
public:
    template <typename... Args>
    explicit HostedViewPanel(const sf::Font& font, Args&&... args)
        : Presenter(std::forward<Args>(args)...), pipeframe::ui::NativeViewPanel(font) {
        const auto size = this->PreferredSize();
        if (size.x != 0 || size.y != 0) this->SetSize({size.x, size.y});
        this->BindHitTester([this](pipeframe::Vector2f point, std::string_view prefix) -> std::optional<std::string> {
            for (auto* hit = this->FindTopmostAt({point.x, point.y}); hit && hit != this; hit = hit->GetParent())
                if (hit->GetKey().starts_with(prefix)) return hit->GetKey();
            return std::nullopt;
        });
    }
    ~HostedViewPanel() override { this->BindHitTester({}); }
    using Presenter::InvalidateView;

protected:
    pipeframe::ui::View BuildNativeView() override { return this->DescribeView(); }
    void OnUpdate(float seconds) override {
        if (lastRevision != this->ViewRevision()) {
            lastRevision = this->ViewRevision();
            pipeframe::ui::NativeViewPanel::InvalidateView();
        }
        pipeframe::ui::NativeViewPanel::OnUpdate(seconds);
    }

private:
    std::uint64_t lastRevision{};
};
}  // namespace pipeframe::backend::sfml
