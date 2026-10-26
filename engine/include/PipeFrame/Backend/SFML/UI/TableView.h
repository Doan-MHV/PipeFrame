#ifndef PIPEFRAME_UI_TABLE_VIEW_H
#define PIPEFRAME_UI_TABLE_VIEW_H

#include <PipeFrame/Backend/SFML/UI/UITheme.h>
#include <PipeFrame/Backend/SFML/UI/Widget.h>

#include <SFML/Graphics/Text.hpp>
#include <functional>
#include <optional>
#include <string>
#include <vector>

struct TableColumn {
    std::string title;
    float weight = 1.0f;
    bool operator==(const TableColumn&) const = default;
};
struct TableRow {
    std::string id;
    std::vector<std::string> cells;
    bool operator==(const TableRow&) const = default;
};

// Read-only cells with single-row selection. Font must outlive the table.
class TableView final : public Widget {
public:
    explicit TableView(const sf::Font& font, const UITheme& theme = UITheme::Dark());
    // Atomic replacement; rows must have unique nonempty IDs and match the column count.
    void SetData(std::vector<TableColumn> columns, std::vector<TableRow> rows);
    const std::vector<TableColumn>& GetColumns() const;
    const std::vector<TableRow>& GetRows() const;
    void SetSelectedRow(std::optional<std::string> id, bool notify = false);
    std::optional<std::string> GetSelectedRow() const;
    void SetOnSelectionChanged(std::function<void(std::optional<std::string>)> callback);
    void SetScrollOffset(float offset);
    float GetScrollOffset() const;
    float GetMaximumScrollOffset() const;

protected:
    void OnRender(sf::RenderTarget& target) const override;
    void OnGeometryChanged() override;
    void OnOpacityChanged() override;
    void OnEnabledChanged() override;
    void OnKeyboardFocusGained() override;
    void OnKeyboardFocusLost() override;
    bool OnEvent(const sf::Event& event) override;

private:
    struct Cell {
        sf::FloatRect clip;
        sf::Text text;
    };
    struct Fill {
        sf::FloatRect bounds;
        sf::Color color;
    };
    void Rebuild();
    void EnsureSelectedVisible();
    std::optional<std::size_t> RowAt(sf::Vector2f point) const;
    const sf::Font& font;
    UITheme theme;
    std::vector<TableColumn> columns;
    std::vector<TableRow> rows;
    std::vector<Cell> cells;
    std::vector<Fill> fills;
    std::optional<std::string> selected;
    std::optional<std::string> pressed;
    std::function<void(std::optional<std::string>)> onSelectionChanged;
    float scrollOffset = 0;
};

#endif
