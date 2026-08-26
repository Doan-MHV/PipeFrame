#ifndef PIPEFRAME_SIMULATION_PANEL_H
#define PIPEFRAME_SIMULATION_PANEL_H

#include <functional>
#include <string>

#include <SFML/Graphics/Font.hpp>
#include <SFML/System/Vector2.hpp>

#include <PipeFrame/UI/Panel.h>

class Label;
class LabeledNumericField;
class TextButton;

class SimulationPanel : public Panel {
  public:
    using ActionCallback = std::function<void()>;
    using ValueCallback = std::function<void(float)>;

    explicit SimulationPanel(const sf::Font &font);

    void SetOnPlayPause(ActionCallback callback);
    void SetOnSingleStep(ActionCallback callback);
    void SetOnReset(ActionCallback callback);

    void SetOnPositionXCommitted(ValueCallback callback);
    void SetOnPositionYCommitted(ValueCallback callback);
    void SetOnRotationCommitted(ValueCallback callback);

    void SetSimulationState(bool playing, bool previewActive);

    void SetSelectionName(const std::string &name);

    void SetTransformEnabled(bool enabled);
    void SetTransformValues(sf::Vector2f position, float rotation);

  private:
    TextButton *playPauseButton = nullptr;
    TextButton *singleStepButton = nullptr;
    TextButton *resetButton = nullptr;

    Label *selectionLabel = nullptr;

    LabeledNumericField *positionXField = nullptr;
    LabeledNumericField *positionYField = nullptr;
    LabeledNumericField *rotationField = nullptr;
};

#endif