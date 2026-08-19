#ifndef PIPEFRAME_BUTTON_H
#define PIPEFRAME_BUTTON_H

#include <functional>

#include <SFML/Graphics/Color.hpp>

#include <PipeFrame/UI/Panel.h>

enum class ButtonState { Normal, Hovered, Pressed, Disabled };

class Button : public Panel {
  public:
    using ClickCallback = std::function<void()>;

    Button();

    void SetOnClick(ClickCallback callback);

    void SetNormalColor(sf::Color color);
    void SetHoveredColor(sf::Color color);
    void SetPressedColor(sf::Color color);
    void SetDisabledColor(sf::Color color);

    ButtonState GetState() const;

  protected:
    bool OnEvent(const sf::Event &event) override;

    void OnPointerEntered() override;
    void OnPointerExited() override;
    void OnEnabledChanged() override;

  private:
    void RefreshVisual();

    ClickCallback onClick;

    sf::Color normalColor{37, 41, 51};
    sf::Color hoveredColor{52, 59, 74};
    sf::Color pressedColor{82, 96, 128};
    sf::Color disabledColor{40, 42, 48};

    ButtonState state = ButtonState::Normal;

    bool pointerInside = false;
    bool pressed = false;
};

#endif