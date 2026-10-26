#include <PipeFrame/Backend/SFML/UI/Badge.h>

#include <PipeFrame/Backend/SFML/UI/Label.h>

Badge::Badge(const sf::Font &font, const std::string &text, const UITheme &theme)
    : Surface(SurfaceVariant::Elevated, theme) {
    SetSize({64.0f, 24.0f});
    SetCornerRadius(theme.radiusPill);
    SetShadowColor(sf::Color::Transparent);
    SetHitTestVisible(false);

    label = &CreateChild<Label>(font);
    label->SetSize(GetSize());
    label->SetText(text);
    label->SetCharacterSize(theme.captionTextSize);
    label->SetAlignment(LabelAlignment::Center);
    label->SetColor(theme.textPrimary);
}

void Badge::SetText(const std::string &text) { label->SetText(text); }

void Badge::SetTextColor(const sf::Color color) { label->SetColor(color); }

void Badge::OnGeometryChanged() {
    Surface::OnGeometryChanged();
    if (label != nullptr) {
        label->Arrange({{0.0f, 0.0f}, GetSize()});
    }
}
