#include <PipeFrame/Backend/SFML/UI/TableView.h>
#include <PipeFrame/Backend/SFML/UI/UIManager.h>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/View.hpp>
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
bool Check(bool value, const char *message) {
    if (!value) std::cerr << "FAILED: " << message << '\n';
    return value;
}
sf::Image Render(const Widget &widget, bool scaled = false) {
    sf::RenderTexture target({360, 200});
    if (scaled) target.setView(sf::View(sf::FloatRect{{0, 0}, {180, 100}}));
    target.clear(sf::Color::Black);
    widget.Render(target);
    target.display();
    return target.getTexture().copyToImage();
}
}
int main() {
    sf::Font font;
    if (!font.openFromFile(PIPEFRAME_TABLE_TEST_FONT)) return 1;
    UIManager manager;
    auto &table = manager.CreateRoot<TableView>(font);
    table.SetPosition({20, 20});
    table.SetSize({300, 144});
    std::vector<TableRow> rows;
    for (int i = 0; i < 20; ++i) rows.push_back({std::to_string(i), {"Long Unicode — candidate αβγ", std::to_string(i)}});
    table.SetData({{"Model", 2}, {"Score", 1}}, rows);
    int notifications = 0;
    table.SetOnSelectionChanged([&](auto) { ++notifications; });
    const auto click = [&](sf::Vector2i position, sf::Vector2i release) {
        manager.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, position});
        manager.HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, release});
    };
    bool passed = true;
    click({30, 70}, {30, 70});
    passed &= Check(table.GetSelectedRow() == "0" && notifications == 1, "Click must select the body row once");
    click({30, 105}, {350, 190});
    passed &= Check(table.GetSelectedRow() == "0" && notifications == 1, "Release outside must cancel selection");
    click({30, 30}, {30, 30});
    passed &= Check(table.GetSelectedRow() == "0", "Header clicks must not select scrolled rows");
    manager.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::End});
    passed &= Check(table.GetSelectedRow() == "19" && table.GetScrollOffset() == table.GetMaximumScrollOffset(),
                    "End must select and reveal the final row");
    manager.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::PageUp});
    passed &= Check(table.GetSelectedRow() == "16", "Page Up must move by the visible row count");
    manager.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Home});
    passed &= Check(table.GetSelectedRow() == "0" && table.GetScrollOffset() == 0, "Home must reveal the first row");
    manager.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,{30,105}});
    auto refreshed=table.GetRows(); refreshed[1].cells[1]="updated";
    table.SetData(table.GetColumns(),refreshed);
    manager.HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,{30,105}});
    passed &= Check(table.GetSelectedRow()=="1","Live cell updates must preserve an in-flight row click");
    const auto beforeScroll = Render(table);
    manager.HandleEvent(sf::Event::MouseWheelScrolled{sf::Mouse::Wheel::Vertical, -1, {40, 80}});
    passed &= Check(table.GetScrollOffset() == 36, "Wheel must scroll the table body");
    const auto afterScroll = Render(table);
    bool sameHeader = true;
    for (unsigned int y = 24; y < 54; ++y) for (unsigned int x = 20; x < 320; ++x)
        sameHeader &= beforeScroll.getPixel({x, y}) == afterScroll.getPixel({x, y});
    passed &= Check(sameHeader, "Scrolling must leave the header fixed");
    click({30, 70}, {30, 70});
    passed &= Check(table.GetSelectedRow() == "1", "Hit testing must account for scroll offset");
    std::reverse(rows.begin(), rows.end());
    table.SetData({{"Model", 2}, {"Score", 1}}, rows);
    passed &= Check(table.GetSelectedRow() == "1", "Selection must survive reordering by row ID");
    for (int invalid = 0; invalid < 3; ++invalid) {
        auto bad = rows;
        auto columns = table.GetColumns();
        if (invalid == 0) bad[1].id = bad[0].id;
        if (invalid == 1) bad[0].cells.pop_back();
        if (invalid == 2) columns[0].weight = std::numeric_limits<float>::quiet_NaN();
        bool rejected = false;
        try { table.SetData(columns, bad); } catch (const std::invalid_argument &) { rejected = true; }
        passed &= Check(rejected && table.GetRows().size() == 20 && table.GetSelectedRow() == "1",
                        "Invalid table data must preserve existing data and selection");
    }
    table.SetScrollOffset(15);
    auto image = Render(table);
    passed &= Check(image.getPixel({30, 170}) == sf::Color::Black && image.getPixel({325, 70}) == sf::Color::Black,
                    "Partial rows and long cells must not paint outside the table");
    table.SetData({{"Model", 2}, {"Score", 1}}, {{"only", {"A", "2"}}});
    passed &= Check(!table.GetSelectedRow() && table.GetScrollOffset() == 0, "Shrinking data must clear stale selection and clamp scrolling");
    table.SetSize({120, 72});
    image = Render(table, true);
    passed &= Check(image.getPixel({270, 150}) != sf::Color::Black && image.getPixel({285, 150}) == sf::Color::Black,
                    "Clipping must respect scaled render views");
    table.SetEnabled(false);
    click({30, 70}, {30, 70});
    passed &= Check(!table.GetSelectedRow(), "Disabled tables must not select rows");
    table.SetEnabled(true);
    table.SetSize({120, 0});
    table.SetSelectedRow("only");
    table.SetSize({120, 72});
    passed &= Check(table.GetScrollOffset() == 0,
                    "Selection before layout must not scroll the first row out of view");
    table.SetVisible(false);
    passed &= Check(!manager.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {30, 70}}),
                    "Hidden tables must return input to the world");
    table.SetVisible(true);
    table.SetSize({0, 0});
    passed &= Check(Render(table).getPixel({30, 70}) == sf::Color::Black, "Collapsed tables must clear cached content");
    table.SetData({}, {});
    if (!passed) return 1;
    std::cout << "All table view tests passed.\n";
}
