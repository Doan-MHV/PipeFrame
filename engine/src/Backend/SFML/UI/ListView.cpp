#include <PipeFrame/Backend/SFML/UI/ListView.h>

#include <algorithm>
#include <utility>

#include <SFML/Window/Keyboard.hpp>

ListView::ListView(const UITheme &newTheme) : theme(newTheme) {
    SetFillColor(sf::Color::Transparent);
    SetOutlineThickness(0.0f);
    SetSpacing(theme.spacing2);
}

Button &ListView::AddItem() {
    const std::size_t index = items.size();
    Button &item = CreateChild<Button>();
    item.SetSize({160.0f, theme.controlHeight});
    item.SetSizePolicy(SizePolicy::Stretch, SizePolicy::Fixed);
    item.SetNormalColor(theme.controlNormal);
    item.SetHoveredColor(theme.controlHovered);
    item.SetPressedColor(theme.controlPressed);
    item.SetSelectedColor(theme.controlSelected);
    item.SetOnClick([this, index]() { SetSelectedIndex(index, true); });
    items.push_back(&item);
    return item;
}

std::size_t ListView::GetItemCount() const { return items.size(); }

Button *ListView::GetItem(const std::size_t index) {
    return index < items.size() ? items[index] : nullptr;
}

const Button *ListView::GetItem(const std::size_t index) const {
    return index < items.size() ? items[index] : nullptr;
}

void ListView::SetSelectedIndex(const std::size_t index, const bool notify) {
    if (index >= items.size() || selectedIndex == index) {
        return;
    }
    selectedIndex = index;
    for (std::size_t current = 0; current < items.size(); ++current) {
        items[current]->SetSelected(current == selectedIndex);
    }
    if (notify && onSelectionChanged) {
        onSelectionChanged(selectedIndex);
    }
}

std::size_t ListView::GetSelectedIndex() const { return selectedIndex; }

void ListView::SetOnSelectionChanged(SelectionChangedCallback callback) {
    onSelectionChanged = std::move(callback);
}

bool ListView::OnEvent(const sf::Event &event) {
    const auto *key = event.getIf<sf::Event::KeyPressed>();
    if (key == nullptr || items.empty()) {
        return false;
    }
    if (key->code == sf::Keyboard::Key::Up) {
        SelectRelative(-1);
        return true;
    }
    if (key->code == sf::Keyboard::Key::Down) {
        SelectRelative(1);
        return true;
    }
    if (key->code == sf::Keyboard::Key::Home) {
        SetSelectedIndex(0, true);
        return true;
    }
    if (key->code == sf::Keyboard::Key::End) {
        SetSelectedIndex(items.size() - 1, true);
        return true;
    }
    return false;
}

void ListView::OnEnabledChanged() {
    for (Button *item : items) {
        item->SetEnabled(IsEnabled());
    }
}

void ListView::SelectRelative(const int direction) {
    const std::size_t current = selectedIndex == NoSelection ? 0 : selectedIndex;
    const int last = static_cast<int>(items.size()) - 1;
    const int next = std::clamp(static_cast<int>(current) + direction, 0, last);
    SetSelectedIndex(static_cast<std::size_t>(next), true);
}
