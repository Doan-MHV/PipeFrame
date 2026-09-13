#ifndef PIPEFRAME_BUTTON_H
#define PIPEFRAME_BUTTON_H

#include <functional>

#include <SFML/Graphics/Color.hpp>

#include <PipeFrame/Backend/SFML/UI/Panel.h>
#include <PipeFrame/Backend/SFML/UI/Motion.h>

enum class ButtonState { Normal, Hovered, Pressed, Focused, Selected, Disabled };

class Button : public Panel {
  public:
    using ClickCallback = std::function<void()>;

    Button();

    void SetOnClick(ClickCallback callback);

    void SetNormalColor(sf::Color color);
    void SetHoveredColor(sf::Color color);
    void SetPressedColor(sf::Color color);
    void SetDisabledColor(sf::Color color);
    void SetSelectedColor(sf::Color color);
    void SetFocusedColor(sf::Color color);

    void SetSelected(bool selected);
    bool IsSelected() const;

    void SetTransitionDuration(float seconds);
    void SetReducedMotion(bool reducedMotion) override;
    sf::Color GetVisualColor() const;

    ButtonState GetState() const;

  protected:
    void OnGeometryChanged() override;
    bool OnEvent(const sf::Event &event) override;

    void OnPointerEntered() override;
    void OnPointerExited() override;
    void OnEnabledChanged() override;
    void OnUpdate(float realDeltaSeconds) override;
    void OnKeyboardFocusGained() override;
    void OnKeyboardFocusLost() override;

  private:
    void RefreshVisual();

    ClickCallback onClick;

    sf::Color normalColor{37, 41, 51};
    sf::Color hoveredColor{52, 59, 74};
    sf::Color pressedColor{82, 96, 128};
    sf::Color disabledColor{40, 42, 48};
    sf::Color selectedColor{49, 113, 177};
    sf::Color focusedColor{52, 72, 102};

    pipeframe::ui::AnimatedColor animatedColor{normalColor};
    float transitionDuration = 0.08f;
    bool reducedMotion = false;

    ButtonState state = ButtonState::Normal;

    bool pointerInside = false;
    bool pressed = false;
    bool keyboardPressed = false;
    bool selected = false;
};

#endif
