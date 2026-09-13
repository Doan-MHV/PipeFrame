#include <PipeFrame/Backend/SFML/UI/TextButton.h>

#include <PipeFrame/Backend/SFML/UI/Label.h>

TextButton::TextButton(const sf::Font &font) {
    label = &CreateChild<Label>(font);

    label->SetPosition({0.0f, 0.0f});
    label->SetSize(GetSize());
    label->SetAlignment(LabelAlignment::Center);
}

void TextButton::SetText(const std::string &text) { label->SetText(text); }

std::string TextButton::GetText() const { return label->GetText(); }

void TextButton::SetTextCharacterSize(unsigned int characterSize) { label->SetCharacterSize(characterSize); }

void TextButton::OnGeometryChanged() {
    Panel::OnGeometryChanged();

    if (label != nullptr) {
        label->SetPosition({0.0f, 0.0f});
        label->SetSize(GetSize());
    }
}

void TextButton::SetTextWrap(bool wrap) { wrappedText=wrap; label->SetWrap(wrap); label->SetHeightPolicy(SizePolicy::FitContent); }
void TextButton::SetTextLeading(bool leading) { label->SetAlignment(leading?LabelAlignment::Left:LabelAlignment::Center); }
sf::Vector2f TextButton::OnMeasure(const BoxConstraints &constraints) {
    if (!wrappedText) return Button::OnMeasure(constraints);
    auto measured=label->Measure(constraints); measured.y=std::max(34.0f,measured.y+8); return measured;
}
