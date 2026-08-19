#include <PipeFrame/UI/Label.h>

Label::Label(const sf::Font &font) : text(font, "", 14) {
    text.setFillColor(sf::Color(225, 230, 240));

    // The parent receives mouse interaction through the label.
    SetHitTestVisible(false);

    RefreshTextPosition();
}

void Label::SetText(const std::string &newText) {
    text.setString(newText);
    RefreshTextPosition();
}

void Label::SetCharacterSize(unsigned int newCharacterSize) {
    text.setCharacterSize(newCharacterSize);
    RefreshTextPosition();
}

void Label::SetColor(sf::Color newColor) { text.setFillColor(newColor); }

void Label::SetAlignment(LabelAlignment newAlignment) {
    alignment = newAlignment;
    RefreshTextPosition();
}

void Label::SetHorizontalPadding(float newPadding) {
    horizontalPadding = newPadding;
    RefreshTextPosition();
}

void Label::OnRender(sf::RenderTarget &target) const { target.draw(text); }

void Label::OnGeometryChanged() { RefreshTextPosition(); }

void Label::RefreshTextPosition() {
    const sf::Vector2f widgetPosition = GetScreenPosition();
    const sf::Vector2f widgetSize = GetSize();

    const sf::FloatRect textBounds = text.getLocalBounds();

    float textX = widgetPosition.x + horizontalPadding;

    if (alignment == LabelAlignment::Center) {
        textX = widgetPosition.x + (widgetSize.x - textBounds.size.x) * 0.5f - textBounds.position.x;
    } else if (alignment == LabelAlignment::Right) {
        textX = widgetPosition.x + widgetSize.x - textBounds.size.x - textBounds.position.x - horizontalPadding;
    }

    const float textY = widgetPosition.y + (widgetSize.y - textBounds.size.y) * 0.5f - textBounds.position.y;

    text.setPosition({textX, textY});
}
