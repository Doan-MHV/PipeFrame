#include <PipeFrame/Backend/SFML/UI/CollapsiblePanel.h>

#include <cmath>
#include <stdexcept>

CollapsiblePanel::CollapsiblePanel(const sf::Font &font, const UITheme &theme) : duration(theme.motionNormal) {
    header = &CreateChild<TextButton>(font);
    viewport = &CreateChild<ScrollPanel>();
    content = &viewport->CreateChild<Column>();
    SetFillColor(sf::Color::Transparent);
    SetOutlineThickness(0);
    SetHitTestVisible(false);
    SetSizePolicy(SizePolicy::Stretch, SizePolicy::FitContent);
    header->SetSize({0, theme.controlHeight});
    header->SetOnClick([this] { SetExpanded(!expanded); });
    viewport->SetFillColor(theme.surface);
    viewport->SetOutlineThickness(0);
    content->SetFillColor(sf::Color::Transparent);
    content->SetOutlineThickness(0);
    content->SetPadding(Thickness{theme.spacing8});
    content->SetSpacing(theme.spacing8);
    content->SetSizePolicy(SizePolicy::Stretch, SizePolicy::FitContent);
    viewport->SetContent(*content);
    SetCaption(caption);
    ApplyHeight();
}
TextButton &CollapsiblePanel::GetHeader() { return *header; }
Column &CollapsiblePanel::GetContent() { return *content; }
void CollapsiblePanel::SetCaption(std::string value) {
    caption = std::move(value);
    header->SetText((expanded ? "- " : "+ ") + caption);
}
void CollapsiblePanel::SetExpandedHeight(float value) {
    if (!std::isfinite(value) || value < 0)
        throw std::invalid_argument("Expanded height must be finite and nonnegative");
    expandedHeight = value;
    if (expanded) {
        height.SetTarget(value, IsReducedMotion());
        ApplyHeight();
    }
}
void CollapsiblePanel::SetExpanded(bool value, bool animate) {
    expanded = value;
    height.SetTarget(expanded ? expandedHeight : 0, !animate || IsReducedMotion());
    header->SetSelected(expanded);
    SetCaption(caption);
    ApplyHeight();
}
bool CollapsiblePanel::IsExpanded() const { return expanded; }
void CollapsiblePanel::SetReducedMotion(bool reduced) {
    Column::SetReducedMotion(reduced);
    if (reduced) {
        height.SetTarget(expanded ? expandedHeight : 0, true);
        ApplyHeight();
    }
}
void CollapsiblePanel::OnUpdate(float delta) {
    if (height.Update(delta, IsReducedMotion() ? 0 : duration))
        ApplyHeight();
}
void CollapsiblePanel::OnGeometryChanged() {
    Column::OnGeometryChanged();
    if (!content)
        return;
    const auto desired = content->Measure({{0, 0}, {GetSize().x, std::numeric_limits<float>::infinity()}});
    content->SetSize({GetSize().x, desired.y});
}
void CollapsiblePanel::ApplyHeight() {
    viewport->SetVisible(expanded || height.Get() > 0);
    viewport->SetEnabled(expanded);
    viewport->SetSize({0, height.Get()});
}
