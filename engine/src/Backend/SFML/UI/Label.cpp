#include <PipeFrame/Backend/SFML/UI/Label.h>

#include <algorithm>
#include <cmath>

Label::Label(const sf::Font &font) : text(font, "", 14) {
    RefreshOpacity();

    // The parent receives mouse interaction through the label.
    SetHitTestVisible(false);

    RefreshTextPosition();
}

void Label::SetText(const std::string &newText) {
    if (content == newText)
        return;
    content = newText;
    textDirty = true;
    measurements.clear();
    RefreshTextPosition();
}

std::string Label::GetText() const { return content; }
void Label::SetWrap(bool enabled) {
    if (wrap == enabled)
        return;
    wrap = enabled;
    textDirty = true;
    measurements.clear();
    RefreshTextPosition();
}

void Label::SetCharacterSize(unsigned int newCharacterSize) {
    if (text.getCharacterSize() == newCharacterSize)
        return;
    text.setCharacterSize(newCharacterSize);
    textDirty = true;
    measurements.clear();
    RefreshTextPosition();
}

void Label::SetColor(const sf::Color newColor) {
    color = newColor;
    RefreshOpacity();
}

sf::Color Label::GetColor() const { return color; }

void Label::SetAlignment(LabelAlignment newAlignment) {
    alignment = newAlignment;
    RefreshTextPosition();
}

void Label::SetHorizontalPadding(float newPadding) {
    if (horizontalPadding == newPadding)
        return;
    measurements.clear();
    horizontalPadding = newPadding;
    RefreshTextPosition();
}

void Label::SetRotation(const float degrees) {
    rotationDegrees = degrees;
    text.setRotation(sf::degrees(degrees));
    RefreshTextPosition();
}

void Label::OnRender(sf::RenderTarget &target) const { target.draw(text); }

void Label::OnGeometryChanged() { RefreshTextPosition(); }

void Label::OnOpacityChanged() { RefreshOpacity(); }

void Label::RefreshOpacity() {
    sf::Color displayed = color;
    displayed.a = static_cast<std::uint8_t>(
        std::clamp(std::lround(static_cast<float>(color.a) * GetEffectiveOpacity()), 0l, 255l));
    text.setFillColor(displayed);
}

void Label::RefreshTextPosition() {
    const sf::Vector2f widgetPosition = GetScreenPosition();
    const sf::Vector2f widgetSize = GetSize();

    const float availableWidth = widgetSize.x - horizontalPadding * 2;
    if (textDirty || cachedTextWidth != availableWidth) {
        const auto display = WrappedText(availableWidth);
        text.setString(display);
        cachedTextWidth = availableWidth;
        textDirty = false;
    }
    const sf::FloatRect textBounds = text.getLocalBounds();

    if (std::abs(rotationDegrees) > 0.01f) {
        text.setOrigin(textBounds.position + textBounds.size * 0.5f);
        text.setPosition(widgetPosition + widgetSize * 0.5f);
        return;
    }
    text.setOrigin({0.0f, 0.0f});

    float textX = widgetPosition.x + horizontalPadding;

    if (alignment == LabelAlignment::Center) {
        textX = widgetPosition.x + (widgetSize.x - textBounds.size.x) * 0.5f - textBounds.position.x;
    } else if (alignment == LabelAlignment::Right) {
        textX = widgetPosition.x + widgetSize.x - textBounds.size.x - textBounds.position.x - horizontalPadding;
    }

    const float textY = widgetPosition.y + (widgetSize.y - textBounds.size.y) * 0.5f - textBounds.position.y;

    text.setPosition({textX, textY});
}

sf::String Label::WrappedText(float availableWidth) const {
    sf::String display(content);
    if (wrap && availableWidth > 0 && std::isfinite(availableWidth)) {
        const float available = availableWidth;
        sf::String output, line;
        sf::Text probe = text;
        for (const auto character : display) {
            if (character == '\n') {
                output += line;
                output += '\n';
                line.clear();
                continue;
            }
            line += character;
            probe.setString(line);
            if (probe.getLocalBounds().size.x > available && line.getSize() > 1) {
                std::size_t split = line.getSize() - 1;
                for (std::size_t i = line.getSize() - 1; i > 0; --i)
                    if (line[i] == ' ') {
                        split = i;
                        break;
                    }
                output += line.substring(0, split);
                output += '\n';
                line = line.substring(split + (line[split] == ' ' ? 1 : 0));
            }
        }
        output += line;
        display = output;
    }
    return display;
}

sf::Vector2f Label::OnMeasure(const BoxConstraints &constraints) {
    const float width = wrap ? constraints.maximum.x - horizontalPadding * 2 : 0.0f;
    for (const auto &cached : measurements)
        if (cached.width == width)
            return constraints.Constrain(cached.size);
    auto probe = text;
    probe.setString(WrappedText(width));
    const auto bounds = probe.getLocalBounds();
    const float lineHeight = text.getFont().getLineSpacing(text.getCharacterSize());
    const sf::Vector2f size{bounds.size.x + horizontalPadding * 2, std::max(lineHeight, bounds.size.y) + 8};
    if (measurements.size() >= 8)
        measurements.erase(measurements.begin());
    measurements.push_back({width, size});
    return constraints.Constrain(size);
}
