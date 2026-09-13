#ifndef PIPEFRAME_UI_TAB_VIEW_H
#define PIPEFRAME_UI_TAB_VIEW_H

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

#include <PipeFrame/Backend/SFML/UI/OverlayPanel.h>
#include <PipeFrame/Backend/SFML/UI/SegmentedControl.h>
#include <PipeFrame/Backend/SFML/UI/StackPanel.h>

class TabView final : public Column {
  public:
    using SelectionChangedCallback = std::function<void(std::size_t)>;

    TabView();

    template <typename WidgetType, typename... Arguments>
    WidgetType &AddPage(Arguments &&...arguments) {
        static_assert(std::is_base_of_v<Widget, WidgetType>,
                      "WidgetType must derive from Widget");
        tabs.AddSegment();
        WidgetType &page = pages.CreateChild<WidgetType>(std::forward<Arguments>(arguments)...);
        page.SetSizePolicy(SizePolicy::Stretch, SizePolicy::Stretch);
        pageWidgets.push_back(&page);
        if (pageWidgets.size() == 1) {
            SetSelectedIndex(0, false);
        } else {
            page.SetVisible(false);
        }
        return page;
    }

    SegmentedControl &GetTabBar();
    const SegmentedControl &GetTabBar() const;
    Button *GetTab(std::size_t index);
    const Button *GetTab(std::size_t index) const;
    Widget *GetPage(std::size_t index);
    const Widget *GetPage(std::size_t index) const;
    std::size_t GetPageCount() const;

    void SetSelectedIndex(std::size_t index, bool notify = false);
    std::size_t GetSelectedIndex() const;
    void SetOnSelectionChanged(SelectionChangedCallback callback);

  private:
    void ApplySelection(std::size_t index, bool notify);

    SegmentedControl &tabs;
    OverlayPanel &pages;
    std::vector<Widget *> pageWidgets;
    SelectionChangedCallback onSelectionChanged;
};

#endif
