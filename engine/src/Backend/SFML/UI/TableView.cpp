#include <PipeFrame/Backend/SFML/UI/TableView.h>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/View.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace {
sf::FloatRect PixelScissor(const sf::RenderTarget &target, sf::FloatRect bounds) {
    const auto start = target.mapCoordsToPixel(bounds.position);
    const auto end = target.mapCoordsToPixel(bounds.position + bounds.size);
    const auto size = sf::Vector2f(target.getSize());
    return {{start.x / size.x, start.y / size.y},
            {std::max(0, end.x - start.x) / size.x, std::max(0, end.y - start.y) / size.y}};
}
} // namespace

TableView::TableView(const sf::Font &newFont, const UITheme &newTheme) : font(newFont), theme(newTheme) {
    SetFocusable(true);
    SetSize({320, 240});
}
void TableView::SetData(std::vector<TableColumn> newColumns, std::vector<TableRow> newRows) {
    if (columns == newColumns && rows == newRows)
        return;
    for (const auto &column : newColumns) {
        if (!std::isfinite(column.weight) || column.weight <= 0) {
            throw std::invalid_argument("Table column weights must be finite and positive");
        }
    }
    std::unordered_set<std::string> ids;
    for (const auto &row : newRows) {
        if (newColumns.empty() || row.id.empty() || !ids.insert(row.id).second ||
            row.cells.size() != newColumns.size()) {
            throw std::invalid_argument("Table rows require unique nonempty IDs and one cell per column");
        }
    }
    if (selected && !ids.contains(*selected))
        selected.reset();
    const bool sameRowOrder =
        rows.size() == newRows.size() && std::equal(rows.begin(), rows.end(), newRows.begin(),
                                                    [](const TableRow &a, const TableRow &b) { return a.id == b.id; });
    if (!sameRowOrder)
        pressed.reset();
    columns = std::move(newColumns);
    rows = std::move(newRows);
    scrollOffset = std::clamp(scrollOffset, 0.0f, GetMaximumScrollOffset());
    Rebuild();
}
const std::vector<TableColumn> &TableView::GetColumns() const { return columns; }
const std::vector<TableRow> &TableView::GetRows() const { return rows; }
std::optional<std::string> TableView::GetSelectedRow() const { return selected; }
void TableView::SetSelectedRow(std::optional<std::string> id, bool notify) {
    if (id && std::none_of(rows.begin(), rows.end(), [&](const auto &row) { return row.id == *id; })) {
        throw std::invalid_argument("Selected table row must exist");
    }
    if (selected == id)
        return;
    selected = std::move(id);
    EnsureSelectedVisible();
    Rebuild();
    if (notify && onSelectionChanged)
        onSelectionChanged(selected);
}
void TableView::SetOnSelectionChanged(std::function<void(std::optional<std::string>)> callback) {
    onSelectionChanged = std::move(callback);
}
float TableView::GetScrollOffset() const { return scrollOffset; }
float TableView::GetMaximumScrollOffset() const {
    return std::max(0.0f, static_cast<float>(rows.size()) * theme.controlHeight -
                              std::max(0.0f, GetSize().y - theme.controlHeight));
}
void TableView::SetScrollOffset(float offset) {
    scrollOffset = std::clamp(std::isfinite(offset) ? offset : 0.0f, 0.0f, GetMaximumScrollOffset());
    pressed.reset();
    Rebuild();
}
void TableView::EnsureSelectedVisible() {
    if (!selected)
        return;
    const auto found = std::find_if(rows.begin(), rows.end(), [&](const auto &row) { return row.id == *selected; });
    const float top = static_cast<float>(found - rows.begin()) * theme.controlHeight;
    const float height = std::max(0.0f, GetSize().y - theme.controlHeight);
    if (height <= 0)
        return;
    if (top < scrollOffset)
        scrollOffset = top;
    else if (top + theme.controlHeight > scrollOffset + height)
        scrollOffset = top + theme.controlHeight - height;
    scrollOffset = std::clamp(scrollOffset, 0.0f, GetMaximumScrollOffset());
}
void TableView::OnGeometryChanged() {
    scrollOffset = std::clamp(scrollOffset, 0.0f, GetMaximumScrollOffset());
    Rebuild();
}
void TableView::OnOpacityChanged() { Rebuild(); }
void TableView::OnEnabledChanged() {
    pressed.reset();
    Rebuild();
}
void TableView::OnKeyboardFocusGained() { Rebuild(); }
void TableView::OnKeyboardFocusLost() {
    pressed.reset();
    Rebuild();
}

std::optional<std::size_t> TableView::RowAt(sf::Vector2f point) const {
    if (!Contains(point))
        return std::nullopt;
    const float y = point.y - GetScreenPosition().y - theme.controlHeight;
    if (y < 0)
        return std::nullopt;
    const auto index = static_cast<std::size_t>((y + scrollOffset) / theme.controlHeight);
    return index < rows.size() ? std::optional<std::size_t>(index) : std::nullopt;
}
bool TableView::OnEvent(const sf::Event &event) {
    if (const auto *wheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (wheel->wheel != sf::Mouse::Wheel::Vertical)
            return false;
        SetScrollOffset(scrollOffset - wheel->delta * theme.controlHeight);
        return true;
    }
    if (const auto *press = event.getIf<sf::Event::MouseButtonPressed>()) {
        pressed.reset();
        if (press->button == sf::Mouse::Button::Left) {
            if (auto index = RowAt(sf::Vector2f(press->position)))
                pressed = rows[*index].id;
        }
        return true;
    }
    if (const auto *release = event.getIf<sf::Event::MouseButtonReleased>()) {
        const auto previous = std::exchange(pressed, std::nullopt);
        if (release->button == sf::Mouse::Button::Left && previous) {
            if (auto index = RowAt(sf::Vector2f(release->position)); index && rows[*index].id == *previous) {
                SetSelectedRow(previous, true);
            }
        }
        return true;
    }
    const auto *key = event.getIf<sf::Event::KeyPressed>();
    if (!key || rows.empty())
        return false;
    const auto found = std::find_if(rows.begin(), rows.end(), [&](const auto &row) { return selected == row.id; });
    auto index = found == rows.end() ? std::ptrdiff_t{0} : found - rows.begin();
    const auto page = std::max<std::ptrdiff_t>(
        1, static_cast<std::ptrdiff_t>((GetSize().y - theme.controlHeight) / theme.controlHeight));
    switch (key->code) {
    case sf::Keyboard::Key::Up:
        if (selected)
            --index;
        break;
    case sf::Keyboard::Key::Down:
        if (selected)
            ++index;
        break;
    case sf::Keyboard::Key::Home:
        index = 0;
        break;
    case sf::Keyboard::Key::End:
        index = static_cast<std::ptrdiff_t>(rows.size()) - 1;
        break;
    case sf::Keyboard::Key::PageUp:
        index -= page;
        break;
    case sf::Keyboard::Key::PageDown:
        index += page;
        break;
    default:
        return false;
    }
    index = std::clamp(index, std::ptrdiff_t{0}, static_cast<std::ptrdiff_t>(rows.size()) - 1);
    SetSelectedRow(rows[static_cast<std::size_t>(index)].id, true);
    return true;
}

void TableView::Rebuild() {
    cells.clear();
    fills.clear();
    const auto size = GetSize(), origin = GetScreenPosition();
    if (size.x <= 0 || size.y <= 0)
        return;
    const auto fade = [&](sf::Color color) {
        color.a = static_cast<std::uint8_t>(std::lround(color.a * GetEffectiveOpacity()));
        return color;
    };
    const sf::FloatRect bounds{origin, size};
    const float rowHeight = theme.controlHeight;
    const sf::FloatRect body{origin + sf::Vector2f{0, rowHeight}, {size.x, std::max(0.0f, size.y - rowHeight)}};
    const auto addCell = [&](const std::string &value, sf::FloatRect rect, sf::FloatRect clip, sf::Color color) {
        auto intersection = rect.findIntersection(clip);
        if (!intersection)
            return;
        sf::Text text(font, "", theme.bodyTextSize);
        sf::String full = sf::String::fromUtf8(value.begin(), value.end());
        for (std::size_t i = 0; i < full.getSize(); ++i)
            if (full[i] == '\n' || full[i] == '\r' || full[i] == '\t')
                full[i] = ' ';
        text.setString(full);
        const float available = std::max(0.0f, rect.size.x - 16);
        if (text.getLocalBounds().size.x > available) {
            const sf::String suffix(U"…");
            std::size_t low = 0, high = full.getSize();
            while (low < high) {
                const auto mid = (low + high + 1) / 2;
                text.setString(full.substring(0, mid) + suffix);
                if (text.getLocalBounds().size.x <= available)
                    low = mid;
                else
                    high = mid - 1;
            }
            text.setString(full.substring(0, low) + suffix);
            if (text.getLocalBounds().size.x > available)
                text.setString("");
        }
        const auto local = text.getLocalBounds();
        text.setPosition(rect.position +
                         sf::Vector2f{8 - local.position.x, (rowHeight - local.size.y) * 0.5f - local.position.y});
        text.setFillColor(fade(IsEnabled() ? color : theme.textDisabled));
        cells.push_back({*intersection, std::move(text)});
    };
    fills.push_back({bounds, fade(theme.surface)});
    double total = 0;
    for (const auto &column : columns)
        total += column.weight;
    const auto addRow = [&](float y, const std::vector<std::string> &values, sf::FloatRect clip, sf::Color color) {
        float x = origin.x;
        for (std::size_t c = 0; c < columns.size(); ++c) {
            const float width = static_cast<float>(size.x * (columns[c].weight / total));
            addCell(values[c], {{x, y}, {width, rowHeight}}, clip, color);
            x += width;
        }
    };
    if (body.size.y > 0) {
        const auto first = static_cast<std::size_t>(scrollOffset / rowHeight);
        const auto count = static_cast<std::size_t>(std::ceil(body.size.y / rowHeight)) + 1;
        for (std::size_t r = first; r < std::min(rows.size(), first + count); ++r) {
            const float y = body.position.y + static_cast<float>(r) * rowHeight - scrollOffset;
            const sf::FloatRect rowBounds{{origin.x, y}, {size.x, rowHeight}};
            if (auto clipped = rowBounds.findIntersection(body)) {
                auto color =
                    selected == rows[r].id ? theme.controlSelected : (r % 2 ? theme.elevatedSurface : theme.surface);
                if (!IsEnabled())
                    color = theme.controlDisabled;
                fills.push_back({*clipped, fade(color)});
            }
            addRow(y, rows[r].cells, body, theme.textPrimary);
        }
        if (rows.empty())
            addCell("No rows", {body.position, {size.x, rowHeight}}, body, theme.textSecondary);
    }
    const sf::FloatRect header{origin, {size.x, std::min(rowHeight, size.y)}};
    fills.push_back({header, fade(theme.controlNormal)});
    std::vector<std::string> titles;
    for (const auto &column : columns)
        titles.push_back(column.title);
    addRow(origin.y, titles, header, theme.textSecondary);
    const float maximumScroll = GetMaximumScrollOffset();
    if (maximumScroll > 0 && body.size.y > 0) {
        const float height =
            std::min(body.size.y, std::max(12.0f, body.size.y * body.size.y / (body.size.y + maximumScroll)));
        const float y = body.position.y + (body.size.y - height) * scrollOffset / maximumScroll;
        fills.push_back({{{origin.x + std::max(0.0f, size.x - 3), y}, {std::min(3.0f, size.x), height}},
                         fade(theme.textSecondary)});
    }
    if (HasKeyboardFocus())
        fills.push_back({{origin, {size.x, 2}}, fade(theme.accent)});
}

void TableView::OnRender(sf::RenderTarget &target) const {
    if (target.getSize().x == 0 || target.getSize().y == 0)
        return;
    const auto previous = target.getView();
    const auto applyClip = [&](sf::FloatRect bounds) {
        auto view = previous;
        view.setScissor(previous.getScissor().findIntersection(PixelScissor(target, bounds)).value_or(sf::FloatRect{}));
        target.setView(view);
    };
    applyClip(GetBounds());
    for (const auto &fill : fills) {
        sf::RectangleShape shape(fill.bounds.size);
        shape.setPosition(fill.bounds.position);
        shape.setFillColor(fill.color);
        target.draw(shape);
    }
    for (const auto &cell : cells) {
        applyClip(cell.clip);
        target.draw(cell.text);
    }
    target.setView(previous);
}
