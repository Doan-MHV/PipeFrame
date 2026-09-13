#ifndef PIPEFRAME_UI_PROGRESS_BAR_H
#define PIPEFRAME_UI_PROGRESS_BAR_H

#include <PipeFrame/Backend/SFML/UI/Motion.h>
#include <PipeFrame/Backend/SFML/UI/Panel.h>
#include <PipeFrame/Backend/SFML/UI/UITheme.h>

class ProgressBar final : public Panel {
  public:
    explicit ProgressBar(const UITheme &theme = UITheme::Dark());

    void SetValue(float value, bool animate = true);
    float GetValue() const;
    float GetVisualValue() const;
    void SetTrackColor(sf::Color color);
    void SetFillColorRole(sf::Color color);
    void SetTransitionDuration(float seconds);
    void SetReducedMotion(bool reducedMotion) override;

  protected:
    void OnGeometryChanged() override;
    void OnUpdate(float realDeltaSeconds) override;

  private:
    void RefreshFill();

    Panel &fill;
    pipeframe::ui::AnimatedFloat animatedValue;
    float value = 0.0f;
    float transitionDuration = 0.0f;
};

#endif
