#include <PipeFrame/UI/NumericField.h>

#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

NumericField::NumericField(const sf::Font &font) : text(font, "", 14) {
    SetFocusable(true);

    SetFillColor(sf::Color(24, 27, 34));
    SetOutlineColor(sf::Color(76, 84, 102));
    SetOutlineThickness(1.0f);

    text.setFillColor(sf::Color(225, 230, 240));

    editBuffer = FormatValue(value);

    RefreshText();
    RefreshVisual();
}

void NumericField::SetValue(float newValue) {
    value = newValue;

    if (!HasKeyboardFocus()) {
        editBuffer = FormatValue(value);
        RefreshText();
    }
}

float NumericField::GetValue() const { return value; }

void NumericField::SetOnValueCommitted(ValueCommittedCallback callback) { onValueCommitted = std::move(callback); }

void NumericField::OnRender(sf::RenderTarget &target) const {
    Panel::OnRender(target);
    target.draw(text);
}

void NumericField::OnGeometryChanged() {
    Panel::OnGeometryChanged();
    RefreshTextPosition();
}

bool NumericField::OnEvent(const sf::Event &event) {
    if (const auto *pressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        return pressed->button == sf::Mouse::Button::Left;
    }

    if (const auto *textEntered = event.getIf<sf::Event::TextEntered>()) {
        const char32_t character = textEntered->unicode;

        const bool digit = character >= U'0' && character <= U'9';
        const bool decimal = character == U'.';
        const bool minus = character == U'-';

        if (!digit && !decimal && !minus) {
            return false;
        }

        if (replaceOnNextText) {
            editBuffer.clear();
            replaceOnNextText = false;
        }

        if (decimal && editBuffer.find('.') != std::string::npos) {
            return true;
        }

        if (minus && !editBuffer.empty()) {
            return true;
        }

        editBuffer.push_back(static_cast<char>(character));

        RefreshText();
        return true;
    }

    if (const auto *keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Backspace) {
            if (replaceOnNextText) {
                editBuffer.clear();
                replaceOnNextText = false;
            } else if (!editBuffer.empty()) {
                editBuffer.pop_back();
            }

            RefreshText();
            return true;
        }

        if (keyPressed->code == sf::Keyboard::Key::Enter) {
            Commit();
            return true;
        }

        if (keyPressed->code == sf::Keyboard::Key::Escape) {
            CancelEditing();
            return true;
        }
    }

    return false;
}

void NumericField::OnKeyboardFocusGained() {
    editBuffer = FormatValue(value);
    replaceOnNextText = true;

    SetOutlineColor(sf::Color(100, 150, 230));
    SetOutlineThickness(2.0f);

    RefreshVisual();
    RefreshText();
}

void NumericField::OnKeyboardFocusLost() {
    Commit();
    RefreshVisual();
}

void NumericField::Commit() {
    try {
        std::size_t parsedCharacters = 0;

        const float parsedValue = std::stof(editBuffer, &parsedCharacters);

        const bool valid = parsedCharacters == editBuffer.size() && std::isfinite(parsedValue);

        if (valid) {
            value = parsedValue;

            if (onValueCommitted) {
                onValueCommitted(value);
            }
        }
    } catch (...) {
        // Invalid input restores the previous valid value.
    }

    editBuffer = FormatValue(value);
    replaceOnNextText = true;

    RefreshText();
}

void NumericField::CancelEditing() {
    editBuffer = FormatValue(value);
    replaceOnNextText = true;

    RefreshText();
}

void NumericField::RefreshText() {
    text.setString(editBuffer);
    RefreshTextPosition();
}

void NumericField::RefreshTextPosition() {
    const sf::FloatRect bounds = text.getLocalBounds();

    const sf::Vector2f position = GetScreenPosition();
    const sf::Vector2f size = GetSize();

    const float textX = position.x + 8.0f - bounds.position.x;

    const float textY = position.y + (size.y - bounds.size.y) * 0.5f - bounds.position.y;

    text.setPosition({textX, textY});
}

std::string NumericField::FormatValue(float value) {
    std::ostringstream stream;

    stream << std::fixed << std::setprecision(2) << value;

    return stream.str();
}

void NumericField::OnEnabledChanged() { RefreshVisual(); }

void NumericField::RefreshVisual() {
    if (!IsEnabled()) {
        SetFillColor(sf::Color(35, 37, 44));
        SetOutlineColor(sf::Color(55, 60, 72));
        SetOutlineThickness(1.0f);

        text.setFillColor(sf::Color(105, 110, 125));
        return;
    }

    SetFillColor(sf::Color(24, 27, 34));
    text.setFillColor(sf::Color(225, 230, 240));

    if (HasKeyboardFocus()) {
        SetOutlineColor(sf::Color(100, 150, 230));
        SetOutlineThickness(2.0f);
    } else {
        SetOutlineColor(sf::Color(76, 84, 102));
        SetOutlineThickness(1.0f);
    }
}