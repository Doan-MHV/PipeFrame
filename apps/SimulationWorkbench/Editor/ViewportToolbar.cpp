#include "ViewportToolbar.h"

#include <algorithm>
#include <utility>

#include <SFML/Graphics/Color.hpp>

#include <PipeFrame/UI/Label.h>
#include <PipeFrame/UI/TextButton.h>

ViewportToolbar::ViewportToolbar(const sf::Font &font) {
    SetSize({700.0f, 68.0f});
    SetFillColor(sf::Color(30, 34, 43, 245));
    SetOutlineColor(sf::Color(78, 86, 104));
    SetOutlineThickness(1.0f);

    metricsButton = &CreateChild<TextButton>(font);
    metricsButton->SetPosition({4.0f, 4.0f});
    metricsButton->SetSize({92.0f, 28.0f});
    metricsButton->SetText("METRICS");
    metricsButton->SetTextCharacterSize(11);

    undoButton = &CreateChild<TextButton>(font);
    undoButton->SetPosition({100.0f, 4.0f});
    undoButton->SetSize({68.0f, 28.0f});
    undoButton->SetText("UNDO");
    undoButton->SetTextCharacterSize(10);

    redoButton = &CreateChild<TextButton>(font);
    redoButton->SetPosition({172.0f, 4.0f});
    redoButton->SetSize({68.0f, 28.0f});
    redoButton->SetText("REDO");
    redoButton->SetTextCharacterSize(10);

    saveButton = &CreateChild<TextButton>(font);
    saveButton->SetPosition({244.0f, 4.0f});
    saveButton->SetSize({60.0f, 28.0f});
    saveButton->SetText("SAVE");
    saveButton->SetTextCharacterSize(10);

    loadButton = &CreateChild<TextButton>(font);
    loadButton->SetPosition({308.0f, 4.0f});
    loadButton->SetSize({60.0f, 28.0f});
    loadButton->SetText("LOAD");
    loadButton->SetTextCharacterSize(10);

    titleLabel = &CreateChild<Label>(font);
    titleLabel->SetText("SCENE VIEW");
    titleLabel->SetCharacterSize(13);
    titleLabel->SetAlignment(LabelAlignment::Center);
    titleLabel->SetHitTestVisible(false);

    statusLabel = &CreateChild<Label>(font);
    statusLabel->SetCharacterSize(12);
    statusLabel->SetAlignment(LabelAlignment::Right);
    statusLabel->SetHorizontalPadding(12.0f);
    statusLabel->SetHitTestVisible(false);

    OnGeometryChanged();
}

void ViewportToolbar::SetOnMetrics(ActionCallback callback) { metricsButton->SetOnClick(std::move(callback)); }

void ViewportToolbar::SetOnUndo(ActionCallback callback) { undoButton->SetOnClick(std::move(callback)); }

void ViewportToolbar::SetOnRedo(ActionCallback callback) { redoButton->SetOnClick(std::move(callback)); }

void ViewportToolbar::SetOnSave(ActionCallback callback) { saveButton->SetOnClick(std::move(callback)); }

void ViewportToolbar::SetOnLoad(ActionCallback callback) { loadButton->SetOnClick(std::move(callback)); }

void ViewportToolbar::SetMetricsVisible(bool visible) { metricsButton->SetText(visible ? "CLOSE" : "METRICS"); }

void ViewportToolbar::SetHistoryEnabled(bool undoEnabled, bool redoEnabled) {
    undoButton->SetEnabled(undoEnabled);
    redoButton->SetEnabled(redoEnabled);
}

void ViewportToolbar::SetAuthoringEnabled(bool enabled) {
    saveButton->SetEnabled(enabled);
    loadButton->SetEnabled(enabled);
}

void ViewportToolbar::SetStatusText(const std::string &text) { statusLabel->SetText(text); }

void ViewportToolbar::OnGeometryChanged() {
    if (titleLabel == nullptr || statusLabel == nullptr) {
        return;
    }

    constexpr float ControlsWidth = 372.0f;
    constexpr float RowHeight = 28.0f;

    const float width = GetSize().x;
    const float titleWidth = std::max(1.0f, width - ControlsWidth - 4.0f);

    titleLabel->SetPosition({ControlsWidth, 4.0f});
    titleLabel->SetSize({titleWidth, RowHeight});
    titleLabel->SetVisible(width >= 500.0f);

    statusLabel->SetPosition({4.0f, 36.0f});
    statusLabel->SetSize({std::max(1.0f, width - 8.0f), RowHeight});
}