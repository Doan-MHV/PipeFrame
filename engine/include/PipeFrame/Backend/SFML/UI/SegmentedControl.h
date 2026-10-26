#ifndef PIPEFRAME_UI_SEGMENTED_CONTROL_H
#define PIPEFRAME_UI_SEGMENTED_CONTROL_H

#include <PipeFrame/Backend/SFML/UI/Button.h>
#include <PipeFrame/Backend/SFML/UI/StackPanel.h>
#include <PipeFrame/Backend/SFML/UI/UITheme.h>

#include <cstddef>
#include <functional>
#include <limits>
#include <vector>

class SegmentedControl final : public Row {
public:
    using SelectionChangedCallback = std::function<void(std::size_t)>;
    static constexpr std::size_t NoSelection = std::numeric_limits<std::size_t>::max();

    explicit SegmentedControl(const UITheme& theme = UITheme::Dark());

    Button& AddSegment();
    std::size_t GetSegmentCount() const;
    Button* GetSegment(std::size_t index);
    const Button* GetSegment(std::size_t index) const;

    void SetSelectedIndex(std::size_t index, bool notify = false);
    std::size_t GetSelectedIndex() const;
    void SetOnSelectionChanged(SelectionChangedCallback callback);

protected:
    bool OnEvent(const sf::Event& event) override;
    void OnEnabledChanged() override;

private:
    void SelectRelative(int direction);

    UITheme theme;
    std::vector<Button*> segments;
    SelectionChangedCallback onSelectionChanged;
    std::size_t selectedIndex = NoSelection;
};

#endif
