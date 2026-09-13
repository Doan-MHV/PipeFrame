#ifndef PIPEFRAME_UI_LIST_VIEW_H
#define PIPEFRAME_UI_LIST_VIEW_H

#include <cstddef>
#include <functional>
#include <limits>
#include <vector>

#include <PipeFrame/Backend/SFML/UI/Button.h>
#include <PipeFrame/Backend/SFML/UI/StackPanel.h>
#include <PipeFrame/Backend/SFML/UI/UITheme.h>

class ListView final : public Column {
  public:
    using SelectionChangedCallback = std::function<void(std::size_t)>;
    static constexpr std::size_t NoSelection = std::numeric_limits<std::size_t>::max();

    explicit ListView(const UITheme &theme = UITheme::Dark());

    Button &AddItem();
    std::size_t GetItemCount() const;
    Button *GetItem(std::size_t index);
    const Button *GetItem(std::size_t index) const;

    void SetSelectedIndex(std::size_t index, bool notify = false);
    std::size_t GetSelectedIndex() const;
    void SetOnSelectionChanged(SelectionChangedCallback callback);

  protected:
    bool OnEvent(const sf::Event &event) override;
    void OnEnabledChanged() override;

  private:
    void SelectRelative(int direction);

    UITheme theme;
    std::vector<Button *> items;
    SelectionChangedCallback onSelectionChanged;
    std::size_t selectedIndex = NoSelection;
};

#endif
