#include <PipeFrame/Backend/SFML/UI/LabeledNumericField.h>

#include <utility>

#include <SFML/Graphics/Color.hpp>

#include <PipeFrame/Backend/SFML/UI/Label.h>

LabeledNumericField::LabeledNumericField(
    const sf::Font &font,
    const std::string &captionText) {

    SetFillColor(sf::Color::Transparent);
    SetOutlineColor(sf::Color::Transparent);
    SetOutlineThickness(0.0f);
    SetHitTestVisible(false);

    SetOrientation(StackOrientation::Horizontal);
    SetSpacing(8.0f);

    captionLabel = &CreateChild<Label>(font);

    captionLabel->SetSize({82.0f, 0.0f});
    captionLabel->SetText(captionText);
    captionLabel->SetCharacterSize(11);
    captionLabel->SetAlignment(LabelAlignment::Left);
    captionLabel->SetHorizontalPadding(4.0f);
    captionLabel->SetColor(sf::Color(150, 160, 180));

    field = &CreateChild<NumericField>(font);
    field->SetSize({154.0f, 0.0f});
}

void LabeledNumericField::SetCaption(
    const std::string &caption) {

    if (captionLabel != nullptr) {
        captionLabel->SetText(caption);
    }
}

void LabeledNumericField::SetValue(const float value) {
    field->SetValue(value);
}

float LabeledNumericField::GetValue() const {
    return field->GetValue();
}

void LabeledNumericField::SetOnValueCommitted(
    NumericField::ValueCommittedCallback callback) {

    field->SetOnValueCommitted(std::move(callback));
}

bool LabeledNumericField::IsEditing() const {
    return field->HasKeyboardFocus();
}

void LabeledNumericField::OnEnabledChanged() {
    if (field != nullptr) {
        field->SetEnabled(IsEnabled());
    }
}
void LabeledNumericField::SetCaptionAbove() {
    SetOrientation(StackOrientation::Vertical);
    SetSpacing(4);
    SetSizePolicy(SizePolicy::Stretch,SizePolicy::FitContent);
    captionLabel->SetSize({0,22});
    captionLabel->SetSizePolicy(SizePolicy::Stretch,SizePolicy::FitContent);
    captionLabel->SetWrap(true);
    field->SetSize({0,34});
    field->SetSizePolicy(SizePolicy::Stretch,SizePolicy::Fixed);
    RefreshLayout();
}
