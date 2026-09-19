#pragma once
#include "ViewWrap.h"
#include <PipeFrame/Backend/SFML/GraphicsResourceAccess.h>
#include <PipeFrame/Backend/SFML/UI/Chart.h>
#include <PipeFrame/Backend/SFML/UI/Label.h>
#include <PipeFrame/Backend/SFML/UI/LabeledNumericField.h>
#include <PipeFrame/Backend/SFML/UI/LabeledTextField.h>
#include <PipeFrame/Backend/SFML/UI/OverlayPanel.h>
#include <PipeFrame/Backend/SFML/UI/ProgressBar.h>
#include <PipeFrame/Backend/SFML/UI/ScrollPanel.h>
#include <PipeFrame/Backend/SFML/UI/Slider.h>
#include <PipeFrame/Backend/SFML/UI/StackPanel.h>
#include <PipeFrame/Backend/SFML/UI/TextButton.h>
#include <PipeFrame/Backend/SFML/UI/TextField.h>
#include <PipeFrame/Backend/SFML/UI/UITheme.h>
#include <PipeFrame/Backend/SFML/VertexAccess.h>
#include <PipeFrame/UI/View.h>

namespace pipeframe::ui {
class ViewButton final : public TextButton {
  public:
    using TextButton::TextButton;

  protected:
    bool ClipsChildren() const override { return true; }
};
class ViewMesh final : public Widget {
  public:
    std::vector<View::MeshLayer> layers;
    std::shared_ptr<GraphicsResourceService> resources;
    ViewMesh() { SetHitTestVisible(false); }

  protected:
    void OnRender(sf::RenderTarget &target) const override {
        sf::RenderStates states;
        states.transform.translate(GetScreenPosition() + GetSize() * 0.5f);
        const float extent = std::min(GetSize().x, GetSize().y);
        states.transform.scale({extent, extent});
        for (const auto &layer : layers) {
            states.texture =
                resources ? backend::sfml::GraphicsResourceAccess::Texture(*resources, layer.texture) : nullptr;
            backend::sfml::DrawVertices(target, layer.vertices, sf::PrimitiveType::Triangles, states);
        }
    }
};
class ViewScroll final : public ScrollPanel {
    bool layingOut{false};
    void OnGeometryChanged() override {
        ScrollPanel::OnGeometryChanged();
        LayoutContent();
    }
    void OnChildGeometryChanged(Widget &child) override {
        ScrollPanel::OnChildGeometryChanged(child);
        LayoutContent();
    }

  public:
    void LayoutContent() {
        if (layingOut || !GetContent())
            return;
        layingOut = true;
        auto &content = *GetContent();
        auto constraints = BoxConstraints::Unbounded();
        constraints.maximum.x = GetSize().x;
        const auto size = content.Measure(constraints);
        content.Arrange({{0, -GetScrollOffset()}, {GetSize().x, size.y}});
        SetScrollOffset(GetScrollOffset());
        layingOut = false;
    }
};
// Backend adapter shared by immediate descriptions and mounted views.
class ViewRenderer {
  public:
    ViewRenderer(const sf::Font &font, const UITheme &theme) : font(font), theme(theme) {}
    Widget &Render(Widget &parent, const View &view) const {
        if (auto *old = parent.FindChildByKey(view.key); old && old->CompositionIdentity() != view.instanceIdentity)
            parent.RemoveChild(*old);
        Widget *result{};
        switch (view.kind) {
        case View::Kind::Button: {
            auto &button = parent.ReconcileChild<ViewButton>(view.key, font);
            button.SetText(view.text);
            button.SetTextCharacterSize(12);
            button.SetTextWrap(true);
            button.SetTextLeading(view.leading);
            button.SetNormalColor(theme.controlNormal);
            button.SetHoveredColor(theme.controlHovered);
            button.SetSelectedColor(theme.controlSelected);
            button.SetSelected(view.selected);
            button.SetPressedColor(theme.controlPressed);
            button.SetOutlineColor(theme.border);
            button.SetCornerRadius(theme.radiusMedium);
            button.SetOnClick(view.onPressed);
            result = &button;
            break;
        }
        case View::Kind::Text: {
            auto &text = parent.ReconcileChild<Label>(view.key, font);
            text.SetText(view.text);
            text.SetWrap(true);
            text.SetCharacterSize(12);
            text.SetColor(theme.textPrimary);
            result = &text;
            break;
        }
        case View::Kind::Input: {
            auto &input = parent.ReconcileChild<TextField>(view.key, font);
            input.SetValue(view.text);
            input.SetOnValueCommitted(view.onCommitted);
            result = &input;
            break;
        }
        case View::Kind::NumberField: {
            auto &field = parent.ReconcileChild<LabeledNumericField>(view.key, font, view.text);
            field.SetCaptionAbove();
            field.SetCaption(view.text);
            if (!field.IsEditing())
                field.SetValue(view.value);
            field.SetOnValueCommitted(view.onChanged);
            result = &field;
            break;
        }
        case View::Kind::TextField: {
            auto &field = parent.ReconcileChild<LabeledTextField>(view.key, font, view.text);
            field.SetCaptionAbove();
            field.SetCaption(view.text);
            if (!field.IsEditing())
                field.SetValue(view.inputValue);
            field.SetOnValueCommitted(view.onCommitted);
            result = &field;
            break;
        }
        case View::Kind::Slider: {
            auto &slider = parent.ReconcileChild<::Slider>(view.key, theme);
            slider.SetRange(view.minimum, view.maximum);
            slider.SetStep(view.step);
            slider.SetValue(view.value);
            slider.SetOnValueChanged(view.onChanged);
            result = &slider;
            break;
        }
        case View::Kind::Mesh: {
            auto &mesh = parent.ReconcileChild<ViewMesh>(view.key);
            mesh.layers = view.mesh;
            mesh.resources = view.resources;
            result = &mesh;
            break;
        }
        case View::Kind::Progress: {
            auto &bar = parent.ReconcileChild<ProgressBar>(view.key, theme);
            bar.SetValue(view.value, false);
            result = &bar;
            break;
        }
        case View::Kind::Chart: {
            auto &chart = parent.ReconcileChild<TimeSeriesChart>(view.key, theme);
            std::vector<ChartSeries> series;
            for (const auto &source : view.series) {
                ChartSeries item;
                item.color = {source.color.r, source.color.g, source.color.b, source.color.a};
                for (const auto &sample : source.samples)
                    item.samples.push_back({sample.x, sample.y});
                series.push_back(std::move(item));
            }
            chart.SetSeries(std::move(series));
            result = &chart;
            break;
        }
        case View::Kind::Scroll: {
            auto &scroll = parent.ReconcileChild<ViewScroll>(view.key);
            Clear(scroll);
            const float offset = scroll.GetScrollOffset();
            scroll.BeginCompositionPass(true);
            auto &content = Render(scroll, view.children.at(0));
            scroll.SetContent(content);
            scroll.EndCompositionPass();
            scroll.LayoutContent();
            scroll.SetScrollOffset(offset);
            result = &scroll;
            break;
        }
        case View::Kind::Stack:
        case View::Kind::Popup: {
            auto &stack = parent.ReconcileChild<OverlayPanel>(view.key);
            Clear(stack);
            stack.SetPadding(Thickness{view.padding});
            stack.SetInputBarrier(view.kind == View::Kind::Popup);
            stack.SetHitTestVisible(view.kind == View::Kind::Popup);
            if (view.kind == View::Kind::Popup)
                stack.SetFillColor(theme.scrim);
            stack.BeginCompositionPass(true);
            for (const auto &child : view.children) {
                auto &widget = Render(stack, child);
                stack.SetChildAlignment(widget, view.kind == View::Kind::Popup
                                                    ? Alignment{HorizontalAlignment::Center, VerticalAlignment::Center}
                                                    : Alignment{});
            }
            stack.EndCompositionPass();
            result = &stack;
            break;
        }
        case View::Kind::Wrap: {
            auto &wrap = parent.ReconcileChild<ViewWrap>(view.key);
            Clear(wrap);
            wrap.minimumWidth = view.minimum;
            wrap.gap = view.spacing;
            wrap.BeginCompositionPass(true);
            for (const auto &child : view.children)
                Render(wrap, child);
            wrap.EndCompositionPass();
            result = &wrap;
            break;
        }
        case View::Kind::Row:
        case View::Kind::Column: {
            auto &stack = parent.ReconcileChild<StackPanel>(view.key);
            Clear(stack);
            stack.SetOrientation(view.kind == View::Kind::Row ? StackOrientation::Horizontal
                                                              : StackOrientation::Vertical);
            stack.SetSpacing(view.spacing);
            stack.SetPadding(Thickness{view.padding});
            stack.BeginCompositionPass(true);
            for (const auto &child : view.children) {
                auto &widget = Render(stack, child);
                stack.SetChildFlex(widget, child.flex > 0 ? child.flex
                                                          : (view.kind == View::Kind::Row && child.width == 0 ? 1 : 0));
            }
            stack.EndCompositionPass();
            result = &stack;
            break;
        }
        case View::Kind::Stateful:
            throw std::logic_error("Stateful descriptions require MountView");
        }
        auto &widget = *result;
        if (view.surface)
            if (auto *panel = dynamic_cast<Panel *>(&widget)) {
                panel->SetFillColor(theme.surface);
                panel->SetOutlineColor(theme.border);
                panel->SetOutlineThickness(1);
                panel->SetCornerRadius(theme.radiusMedium);
            }
        widget.SetCompositionIdentity(view.instanceIdentity);
        // Preserve the arranged size when the requested dimensions did not change.
        // Resetting stretch/fit controls to zero forces cascading relayouts.
        if (widget.GetRequestedSize() != sf::Vector2f{view.width, view.height})
            widget.SetSize({view.width, view.height});
        widget.SetSizePolicy(view.width > 0 ? SizePolicy::Fixed : SizePolicy::Stretch,
                             view.stretchHeight ? SizePolicy::Stretch
                                                : (view.fitHeight ? SizePolicy::FitContent : SizePolicy::Fixed));
        widget.SetEnabled(view.enabled);
        widget.SetVisible(true);
        if (auto *wrap = dynamic_cast<ViewWrap *>(&widget))
            wrap->Refresh();
        if (auto *scroll = dynamic_cast<ViewScroll *>(&widget))
            scroll->LayoutContent();
        return widget;
    }

  private:
    static void Clear(Panel &panel) {
        panel.SetFillColor(sf::Color::Transparent);
        panel.SetOutlineThickness(0);
    }
    const sf::Font &font;
    const UITheme &theme;
};
} // namespace pipeframe::ui
