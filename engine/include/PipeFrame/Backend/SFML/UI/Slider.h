#ifndef PIPEFRAME_UI_SLIDER_H
#define PIPEFRAME_UI_SLIDER_H

#include <functional>

#include <PipeFrame/Backend/SFML/UI/Panel.h>
#include <PipeFrame/Backend/SFML/UI/UITheme.h>

class Slider final : public Panel {
  public:
    using ValueChangedCallback = std::function<void(float)>;

    explicit Slider(const UITheme &theme = UITheme::Dark());

    void SetRange(float minimum, float maximum);
    float GetMinimum() const;
    float GetMaximum() const;

    void SetStep(float step);
    float GetStep() const;

    void SetValue(float value);
    float GetValue() const;
    float GetNormalizedValue() const;
    void SetOnValueChanged(ValueChangedCallback callback);

  protected:
    bool OnEvent(const sf::Event &event) override;
    void OnGeometryChanged() override;
    void OnEnabledChanged() override;
    void OnKeyboardFocusGained() override;
    void OnKeyboardFocusLost() override;

  private:
    void SetValueFromInput(float value);
    void SetValueFromPointer(float screenX);
    float SanitizeValue(float value) const;
    void RefreshGeometry();
    void RefreshVisual();

    Panel &track;
    Panel &filledTrack;
    Panel &thumb;
    UITheme theme;
    ValueChangedCallback onValueChanged;
    float minimum = 0.0f;
    float maximum = 1.0f;
    float step = 0.0f;
    float value = 0.0f;
    bool dragging = false;
};

#endif
