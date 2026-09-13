#include <PipeFrame/Backend/SFML/UI/TextField.h>

#include <algorithm>
#include <utility>

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

TextField::TextField(
    const sf::Font &font
)
    : text(font, "", 14) {
    SetFocusable(true);

    SetFillColor({
        24,
        27,
        34,
    });

    SetOutlineColor({
        76,
        84,
        102,
    });

    SetOutlineThickness(1.0f);

    text.setFillColor({
        225,
        230,
        240,
    });

    RefreshText();
    RefreshVisual();
}

void TextField::SetValue(
    const std::string &newValue
) {
    value = newValue;

    if (!HasKeyboardFocus()) {
        editBuffer = value;
        RefreshText();
    }
}

const std::string &
TextField::GetValue() const {
    return value;
}

void TextField::SetOnValueCommitted(
    ValueCommittedCallback callback
) {
    onValueCommitted =
        std::move(callback);
}

void TextField::OnRender(
    sf::RenderTarget &target
) const {
    Panel::OnRender(target);
    target.draw(text);
}

void TextField::OnGeometryChanged() {
    Panel::OnGeometryChanged();
    RefreshText();
}

bool TextField::OnEvent(
    const sf::Event &event
) {
    if (const auto *pressed =
            event.getIf<
                sf::Event::MouseButtonPressed>()) {
        return
            pressed->button ==
            sf::Mouse::Button::Left;
    }

    if (const auto *textEntered =
            event.getIf<
                sf::Event::TextEntered>()) {
        const char32_t character =
            textEntered->unicode;

        // Current project paths use UTF-8-compatible ASCII.
        // Full Unicode text handling can be generalized later.
        if (character < U' ' ||
            character > U'~') {
            return false;
        }

        if (replaceOnNextText) {
            editBuffer.clear();
            replaceOnNextText = false;
        }

        editBuffer.push_back(
            static_cast<char>(character));

        RefreshText();
        return true;
    }

    if (const auto *pressed =
            event.getIf<
                sf::Event::KeyPressed>()) {
        if (pressed->code ==
            sf::Keyboard::Key::Backspace) {
            if (replaceOnNextText) {
                editBuffer.clear();
                replaceOnNextText = false;
            } else if (!editBuffer.empty()) {
                editBuffer.pop_back();
            }

            RefreshText();
            return true;
        }

        if (pressed->code ==
            sf::Keyboard::Key::Enter) {
            Commit();
            return true;
        }

        if (pressed->code ==
            sf::Keyboard::Key::Escape) {
            CancelEditing();
            return true;
        }
    }

    return false;
}

void TextField::OnKeyboardFocusGained() {
    editBuffer = value;
    replaceOnNextText = true;

    RefreshVisual();
    RefreshText();
}

void TextField::OnKeyboardFocusLost() {
    Commit();
    RefreshVisual();
}

void TextField::OnEnabledChanged() {
    RefreshVisual();
}

void TextField::Commit() {
    value = editBuffer;
    replaceOnNextText = true;

    if (onValueCommitted) {
        onValueCommitted(value);
    }

    RefreshText();
}

void TextField::CancelEditing() {
    editBuffer = value;
    replaceOnNextText = true;

    RefreshText();
}

void TextField::RefreshText() {
    text.setString(GetDisplayText());
    RefreshTextPosition();
}

void TextField::RefreshTextPosition() {
    const sf::FloatRect bounds =
        text.getLocalBounds();

    const sf::Vector2f position =
        GetScreenPosition();

    const sf::Vector2f size =
        GetSize();

    text.setPosition({
        position.x +
            8.0f -
            bounds.position.x,

        position.y +
            (size.y - bounds.size.y) *
                0.5f -
            bounds.position.y,
    });
}

void TextField::RefreshVisual() {
    if (!IsEnabled()) {
        SetFillColor({
            35,
            37,
            44,
        });

        SetOutlineColor({
            55,
            60,
            72,
        });

        SetOutlineThickness(1.0f);

        text.setFillColor({
            105,
            110,
            125,
        });

        return;
    }

    SetFillColor({
        24,
        27,
        34,
    });

    text.setFillColor({
        225,
        230,
        240,
    });

    if (HasKeyboardFocus()) {
        SetOutlineColor({
            100,
            150,
            230,
        });

        SetOutlineThickness(2.0f);
    } else {
        SetOutlineColor({
            76,
            84,
            102,
        });

        SetOutlineThickness(1.0f);
    }
}

std::string TextField::GetDisplayText() const {
    const std::string &source =
        HasKeyboardFocus()
            ? editBuffer
            : value;

    const float availableWidth =
        std::max(
            1.0f,
            GetSize().x - 16.0f);

    const std::size_t maximumCharacters =
        std::max<std::size_t>(
            4,
            static_cast<std::size_t>(
                availableWidth / 8.0f));

    if (source.size() <=
        maximumCharacters) {
        return source;
    }

    return
        "..." +
        source.substr(
            source.size() -
            (maximumCharacters - 3));
}
