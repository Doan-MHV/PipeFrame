#include <PipeFrame/Backend/SFML/UI/SegmentedControl.h>

#include <algorithm>
#include <utility>

#include <SFML/Window/Keyboard.hpp>

SegmentedControl::SegmentedControl(const UITheme &newTheme) : theme(newTheme) {
    SetSize({160.0f, theme.controlHeight});
    SetSpacing(theme.spacing2);
    SetPadding(Thickness{theme.spacing2});
    SetCornerRadius(theme.radiusMedium);
    SetFillColor(theme.inputBackground);
    SetOutlineColor(theme.subtleBorder);
    SetOutlineThickness(theme.borderThickness);
}

Button &SegmentedControl::AddSegment() {
    const std::size_t index = segments.size();
    Button &segment = CreateChild<Button>();
    segment.SetSize({80.0f, std::max(1.0f, GetSize().y - theme.spacing4)});
    segment.SetNormalColor(theme.controlNormal);
    segment.SetHoveredColor(theme.controlHovered);
    segment.SetPressedColor(theme.controlPressed);
    segment.SetSelectedColor(theme.controlSelected);
    segment.SetOnClick([this, index]() { SetSelectedIndex(index, true); });
    SetChildFlex(segment, 1.0f);
    segments.push_back(&segment);
    if (selectedIndex == NoSelection) {
        SetSelectedIndex(0);
    }
    return segment;
}

std::size_t SegmentedControl::GetSegmentCount() const { return segments.size(); }

Button *SegmentedControl::GetSegment(const std::size_t index) {
    return index < segments.size() ? segments[index] : nullptr;
}

const Button *SegmentedControl::GetSegment(const std::size_t index) const {
    return index < segments.size() ? segments[index] : nullptr;
}

void SegmentedControl::SetSelectedIndex(const std::size_t index, const bool notify) {
    if (index >= segments.size() || selectedIndex == index) {
        return;
    }
    selectedIndex = index;
    for (std::size_t current = 0; current < segments.size(); ++current) {
        segments[current]->SetSelected(current == selectedIndex);
    }
    if (notify && onSelectionChanged) {
        onSelectionChanged(selectedIndex);
    }
}

std::size_t SegmentedControl::GetSelectedIndex() const { return selectedIndex; }

void SegmentedControl::SetOnSelectionChanged(SelectionChangedCallback callback) {
    onSelectionChanged = std::move(callback);
}

bool SegmentedControl::OnEvent(const sf::Event &event) {
    const auto *key = event.getIf<sf::Event::KeyPressed>();
    if (key == nullptr || segments.empty()) {
        return false;
    }
    if (key->code == sf::Keyboard::Key::Left || key->code == sf::Keyboard::Key::Up) {
        SelectRelative(-1);
        return true;
    }
    if (key->code == sf::Keyboard::Key::Right || key->code == sf::Keyboard::Key::Down) {
        SelectRelative(1);
        return true;
    }
    if (key->code == sf::Keyboard::Key::Home) {
        SetSelectedIndex(0, true);
        return true;
    }
    if (key->code == sf::Keyboard::Key::End) {
        SetSelectedIndex(segments.size() - 1, true);
        return true;
    }
    return false;
}

void SegmentedControl::OnEnabledChanged() {
    for (Button *segment : segments) {
        segment->SetEnabled(IsEnabled());
    }
}

void SegmentedControl::SelectRelative(const int direction) {
    const std::size_t current = selectedIndex == NoSelection ? 0 : selectedIndex;
    const int count = static_cast<int>(segments.size());
    const int wrapped = (static_cast<int>(current) + direction + count) % count;
    SetSelectedIndex(static_cast<std::size_t>(wrapped), true);
}
