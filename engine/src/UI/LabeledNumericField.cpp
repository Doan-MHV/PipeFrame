#include <PipeFrame/UI/LabeledNumericField.h>

#include <utility>

#include <SFML/Graphics/Color.hpp>

#include <PipeFrame/UI/Label.h>

LabeledNumericField::LabeledNumericField(const sf::Font &font, const std::string &captionText) {
    SetFillColor(sf::Color::Transparent);
    SetOutlineColor(sf::Color::Transparent);
    SetOutlineThickness(0.0f);
    SetHitTestVisible(false);

    SetOrientation(StackOrientation::Horizontal);
    SetSpacing(8.0f);

    Label &caption = CreateChild<Label>(font);

    caption.SetSize({82.0f, 0.0f});
    caption.SetText(captionText);
    caption.SetCharacterSize(11);
    caption.SetAlignment(LabelAlignment::Left);
    caption.SetHorizontalPadding(4.0f);
    caption.SetColor(sf::Color(150, 160, 180));

    field = &CreateChild<NumericField>(font);
    field->SetSize({154.0f, 0.0f});
}

void LabeledNumericField::SetValue(float value) { field->SetValue(value); }

float LabeledNumericField::GetValue() const { return field->GetValue(); }

void LabeledNumericField::SetOnValueCommitted(NumericField::ValueCommittedCallback callback) {
    field->SetOnValueCommitted(std::move(callback));
}

bool LabeledNumericField::IsEditing() const { return field->HasKeyboardFocus(); }

void LabeledNumericField::OnEnabledChanged() {
    if (field != nullptr) {
        field->SetEnabled(IsEnabled());
    }
}