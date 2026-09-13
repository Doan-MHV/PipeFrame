#include <PipeFrame/Backend/SFML/UI/TabView.h>

#include <utility>

TabView::TabView()
    : tabs(CreateChild<SegmentedControl>()), pages(CreateChild<OverlayPanel>()) {
    SetFillColor(sf::Color::Transparent);
    SetOutlineThickness(0.0f);
    SetSpacing(8.0f);
    tabs.SetSizePolicy(SizePolicy::Stretch, SizePolicy::Fixed);
    pages.SetSizePolicy(SizePolicy::Stretch, SizePolicy::Stretch);
    pages.SetFillColor(sf::Color::Transparent);
    pages.SetOutlineThickness(0.0f);
    pages.SetHitTestVisible(false);
    SetChildFlex(pages, 1.0f);
    tabs.SetOnSelectionChanged(
        [this](const std::size_t index) { ApplySelection(index, true); });
}

SegmentedControl &TabView::GetTabBar() { return tabs; }
const SegmentedControl &TabView::GetTabBar() const { return tabs; }
Button *TabView::GetTab(const std::size_t index) { return tabs.GetSegment(index); }
const Button *TabView::GetTab(const std::size_t index) const { return tabs.GetSegment(index); }
Widget *TabView::GetPage(const std::size_t index) {
    return index < pageWidgets.size() ? pageWidgets[index] : nullptr;
}
const Widget *TabView::GetPage(const std::size_t index) const {
    return index < pageWidgets.size() ? pageWidgets[index] : nullptr;
}
std::size_t TabView::GetPageCount() const { return pageWidgets.size(); }

void TabView::SetSelectedIndex(const std::size_t index, const bool notify) {
    if (index >= pageWidgets.size()) {
        return;
    }
    tabs.SetSelectedIndex(index, false);
    ApplySelection(index, notify);
}

std::size_t TabView::GetSelectedIndex() const { return tabs.GetSelectedIndex(); }

void TabView::SetOnSelectionChanged(SelectionChangedCallback callback) {
    onSelectionChanged = std::move(callback);
}

void TabView::ApplySelection(const std::size_t index, const bool notify) {
    if (index >= pageWidgets.size()) {
        return;
    }
    for (std::size_t pageIndex = 0; pageIndex < pageWidgets.size(); ++pageIndex) {
        pageWidgets[pageIndex]->SetVisible(pageIndex == index);
    }
    if (notify && onSelectionChanged) {
        onSelectionChanged(index);
    }
}
