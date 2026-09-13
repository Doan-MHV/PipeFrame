#include <PipeFrame/Backend/SFML/UI/LabeledTextField.h>

#include <utility>

#include <SFML/Graphics/Color.hpp>

#include <PipeFrame/Backend/SFML/UI/Label.h>

LabeledTextField::LabeledTextField(
    const sf::Font &font,
    const std::string &captionText
) {
    SetFillColor(sf::Color::Transparent);
    SetOutlineColor(sf::Color::Transparent);
    SetOutlineThickness(0.0f);
    SetHitTestVisible(false);

    SetOrientation(
        StackOrientation::Horizontal);

    SetSpacing(8.0f);

    captionLabel =
        &CreateChild<Label>(font);

    captionLabel->SetSize({
        82.0f,
        0.0f,
    });

    captionLabel->SetText(captionText);
    captionLabel->SetCharacterSize(11);
    captionLabel->SetAlignment(
        LabelAlignment::Left);

    captionLabel->SetHorizontalPadding(4.0f);

    captionLabel->SetColor({
        150,
        160,
        180,
    });

    field =
        &CreateChild<TextField>(font);

    field->SetSize({
        154.0f,
        0.0f,
    });
}

void LabeledTextField::SetCaption(
    const std::string &caption
) {
    captionLabel->SetText(caption);
}

void LabeledTextField::SetValue(
    const std::string &value
) {
    field->SetValue(value);
}

const std::string &
LabeledTextField::GetValue() const {
    return field->GetValue();
}

void LabeledTextField::SetOnValueCommitted(
    TextField::ValueCommittedCallback callback
) {
    field->SetOnValueCommitted(
        std::move(callback));
}

bool LabeledTextField::IsEditing() const {
    return field->HasKeyboardFocus();
}

void LabeledTextField::OnEnabledChanged() {
    if (field != nullptr) {
        field->SetEnabled(IsEnabled());
    }
}

void LabeledTextField::SetCaptionAbove() {
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
