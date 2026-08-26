#include "SimulationPanel.h"

#include <utility>

#include <SFML/Graphics/Color.hpp>

#include <PipeFrame/UI/Label.h>
#include <PipeFrame/UI/LabeledNumericField.h>
#include <PipeFrame/UI/StackPanel.h>
#include <PipeFrame/UI/TextButton.h>

SimulationPanel::SimulationPanel(const sf::Font &font) {
    SetSize({300.0f, 560.0f});
    SetFillColor(sf::Color(24, 27, 34, 245));
    SetOutlineColor(sf::Color(78, 86, 104));
    SetOutlineThickness(1.0f);

    Panel &headerPanel = CreateChild<Panel>();

    headerPanel.SetPosition({12.0f, 12.0f});
    headerPanel.SetSize({276.0f, 48.0f});
    headerPanel.SetFillColor(sf::Color(42, 47, 59));
    headerPanel.SetOutlineColor(sf::Color(90, 100, 120));
    headerPanel.SetOutlineThickness(1.0f);

    Label &headerLabel = headerPanel.CreateChild<Label>(font);

    headerLabel.SetSize(headerPanel.GetSize());
    headerLabel.SetText("SIMULATION");
    headerLabel.SetCharacterSize(15);
    headerLabel.SetAlignment(LabelAlignment::Left);
    headerLabel.SetHorizontalPadding(12.0f);

    StackPanel &contentPanel = CreateChild<StackPanel>();

    contentPanel.SetPosition({12.0f, 72.0f});
    contentPanel.SetSize({276.0f, 476.0f});
    contentPanel.SetFillColor(sf::Color(31, 34, 43));
    contentPanel.SetOutlineColor(sf::Color(65, 72, 88));
    contentPanel.SetOutlineThickness(1.0f);
    contentPanel.SetOrientation(StackOrientation::Vertical);
    contentPanel.SetPadding(Thickness{12.0f});
    contentPanel.SetSpacing(8.0f);

    playPauseButton = &contentPanel.CreateChild<TextButton>(font);
    playPauseButton->SetSize({0.0f, 44.0f});
    playPauseButton->SetTextCharacterSize(14);

    singleStepButton = &contentPanel.CreateChild<TextButton>(font);
    singleStepButton->SetSize({0.0f, 44.0f});
    singleStepButton->SetText("SINGLE STEP");
    singleStepButton->SetTextCharacterSize(14);

    resetButton = &contentPanel.CreateChild<TextButton>(font);
    resetButton->SetSize({0.0f, 44.0f});
    resetButton->SetText("RESET");
    resetButton->SetTextCharacterSize(14);

    selectionLabel = &contentPanel.CreateChild<Label>(font);
    selectionLabel->SetSize({0.0f, 36.0f});
    selectionLabel->SetText("SELECTED: NONE");
    selectionLabel->SetCharacterSize(12);
    selectionLabel->SetAlignment(LabelAlignment::Left);
    selectionLabel->SetHorizontalPadding(8.0f);
    selectionLabel->SetColor(sf::Color(180, 190, 210));

    Label &transformHeader = contentPanel.CreateChild<Label>(font);

    transformHeader.SetSize({0.0f, 28.0f});
    transformHeader.SetText("TRANSFORM");
    transformHeader.SetCharacterSize(12);
    transformHeader.SetAlignment(LabelAlignment::Left);
    transformHeader.SetHorizontalPadding(8.0f);
    transformHeader.SetColor(sf::Color(225, 230, 240));

    positionXField = &contentPanel.CreateChild<LabeledNumericField>(font, "POSITION X");
    positionXField->SetSize({0.0f, 36.0f});

    positionYField = &contentPanel.CreateChild<LabeledNumericField>(font, "POSITION Y");
    positionYField->SetSize({0.0f, 36.0f});

    rotationField = &contentPanel.CreateChild<LabeledNumericField>(font, "ROTATION");
    rotationField->SetSize({0.0f, 36.0f});

    SetTransformEnabled(false);
    SetSimulationState(false, false);
}

void SimulationPanel::SetOnPlayPause(ActionCallback callback) { playPauseButton->SetOnClick(std::move(callback)); }

void SimulationPanel::SetOnSingleStep(ActionCallback callback) { singleStepButton->SetOnClick(std::move(callback)); }

void SimulationPanel::SetOnReset(ActionCallback callback) { resetButton->SetOnClick(std::move(callback)); }

void SimulationPanel::SetOnPositionXCommitted(ValueCallback callback) {
    positionXField->SetOnValueCommitted(std::move(callback));
}

void SimulationPanel::SetOnPositionYCommitted(ValueCallback callback) {
    positionYField->SetOnValueCommitted(std::move(callback));
}

void SimulationPanel::SetOnRotationCommitted(ValueCallback callback) {
    rotationField->SetOnValueCommitted(std::move(callback));
}

void SimulationPanel::SetSimulationState(bool playing, bool previewActive) {
    playPauseButton->SetText(playing ? "PAUSE" : "PLAY");
    singleStepButton->SetEnabled(!playing);
    resetButton->SetEnabled(!playing && previewActive);
}

void SimulationPanel::SetSelectionName(const std::string &name) {
    if (name.empty()) {
        selectionLabel->SetText("SELECTED: NONE");
        return;
    }

    selectionLabel->SetText("SELECTED: " + name);
}

void SimulationPanel::SetTransformEnabled(bool enabled) {
    positionXField->SetEnabled(enabled);
    positionYField->SetEnabled(enabled);
    rotationField->SetEnabled(enabled);
}

void SimulationPanel::SetTransformValues(sf::Vector2f position, float rotation) {
    if (!positionXField->IsEditing()) {
        positionXField->SetValue(position.x);
    }

    if (!positionYField->IsEditing()) {
        positionYField->SetValue(position.y);
    }

    if (!rotationField->IsEditing()) {
        rotationField->SetValue(rotation);
    }
}