#ifndef PIPEFRAME_UI_METRIC_CARD_H
#define PIPEFRAME_UI_METRIC_CARD_H

#include <PipeFrame/Backend/SFML/UI/Surface.h>

#include <SFML/Graphics/Font.hpp>
#include <string>

class Label;

class MetricCard final : public Surface {
public:
    explicit MetricCard(const sf::Font& font, const UITheme& theme = UITheme::Dark());

    void SetTitle(const std::string& text);
    void SetValueText(const std::string& text);
    void SetDetail(const std::string& text);
    void SetAccentColor(sf::Color color);

    const Label& GetTitleLabel() const;
    const Label& GetValueLabel() const;
    const Label& GetDetailLabel() const;

protected:
    void OnGeometryChanged() override;

private:
    Panel* accent = nullptr;
    Label* title = nullptr;
    Label* value = nullptr;
    Label* detail = nullptr;
    UITheme theme;
};

#endif
