#include <PipeFrame/UI/TextButton.h>

#include <PipeFrame/UI/Label.h>

TextButton::TextButton(const sf::Font &font) {
    label = &CreateChild<Label>(font);

    label->SetPosition({0.0f, 0.0f});
    label->SetSize(GetSize());
    label->SetAlignment(LabelAlignment::Center);
}

void TextButton::SetText(const std::string &text) { label->SetText(text); }

void TextButton::SetTextCharacterSize(unsigned int characterSize) { label->SetCharacterSize(characterSize); }

void TextButton::OnGeometryChanged() {
    Panel::OnGeometryChanged();

    if (label != nullptr) {
        label->SetPosition({0.0f, 0.0f});
        label->SetSize(GetSize());
    }
}