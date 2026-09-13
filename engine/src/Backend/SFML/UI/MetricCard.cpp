#include <PipeFrame/Backend/SFML/UI/MetricCard.h>

#include <algorithm>

#include <PipeFrame/Backend/SFML/UI/Label.h>

MetricCard::MetricCard(const sf::Font &font, const UITheme &newTheme)
    : Surface(SurfaceVariant::Glass, newTheme), theme(newTheme) {
    accent = &CreateChild<Panel>();
    title = &CreateChild<Label>(font);
    value = &CreateChild<Label>(font);
    detail = &CreateChild<Label>(font);
    SetSize({180.0f, 88.0f});
    SetShadowOffset(theme.shadowOffsetSmall);

    accent->SetFillColor(theme.accent);
    accent->SetOutlineColor(sf::Color::Transparent);
    accent->SetOutlineThickness(0.0f);
    accent->SetCornerRadius(theme.radiusPill);
    accent->SetHitTestVisible(false);

    title->SetCharacterSize(theme.captionTextSize);
    title->SetColor(theme.textSecondary);
    title->SetHorizontalPadding(0.0f);
    value->SetCharacterSize(theme.headingTextSize + 2);
    value->SetColor(theme.textPrimary);
    value->SetHorizontalPadding(0.0f);
    detail->SetCharacterSize(theme.captionTextSize);
    detail->SetColor(theme.textSecondary);
    detail->SetHorizontalPadding(0.0f);
    OnGeometryChanged();
}

void MetricCard::SetTitle(const std::string &text) { title->SetText(text); }
void MetricCard::SetValueText(const std::string &text) { value->SetText(text); }
void MetricCard::SetDetail(const std::string &text) {
    detail->SetText(text);
    detail->SetVisible(!text.empty());
}
void MetricCard::SetAccentColor(const sf::Color color) { accent->SetFillColor(color); }
const Label &MetricCard::GetTitleLabel() const { return *title; }
const Label &MetricCard::GetValueLabel() const { return *value; }
const Label &MetricCard::GetDetailLabel() const { return *detail; }

void MetricCard::OnGeometryChanged() {
    Surface::OnGeometryChanged();
    const float height = GetSize().y;
    const float contentX = theme.spacing16;
    const float contentWidth = std::max(0.0f, GetSize().x - contentX - theme.spacing12);
    if (accent == nullptr || title == nullptr || value == nullptr || detail == nullptr) {
        return;
    }
    accent->Arrange({{theme.spacing8, theme.spacing12},
                     {3.0f, std::max(0.0f, height - theme.spacing24)}});
    title->Arrange({{contentX, theme.spacing8}, {contentWidth, 20.0f}});
    value->Arrange({{contentX, 27.0f}, {contentWidth, 30.0f}});
    detail->Arrange({{contentX, std::max(54.0f, height - 26.0f)},
                     {contentWidth, 18.0f}});
}
