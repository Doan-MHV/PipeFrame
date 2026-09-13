#ifndef PIPEFRAME_UI_SIMULATION_TRANSPORT_H
#define PIPEFRAME_UI_SIMULATION_TRANSPORT_H

#include <functional>
#include <string>

#include <SFML/Graphics/Font.hpp>

#include <PipeFrame/Simulation/SimulationController.h>
#include <PipeFrame/UI/State.h>
#include <PipeFrame/Backend/SFML/ViewPanelHost.h>
#include <PipeFrame/UI/SimulationTransportView.h>

class Column;
class Row;
class TextButton;

class SimulationTransport final : public pipeframe::ui::NativeViewPanel {
  public:
    using ActionCallback = std::function<void()>;
    using SpeedSelectedCallback = std::function<void(SimulationSpeed)>;

    explicit SimulationTransport(const sf::Font &font);

    void SetOnPlayPause(ActionCallback callback);
    void SetOnSingleStep(ActionCallback callback);
    void SetOnReset(ActionCallback callback);
    void SetOnSpeedSelected(SpeedSelectedCallback callback);

    void SetSimulationState(bool playing, bool previewActive, SimulationSpeed speed);

    bool IsStepEnabled() const;
    bool IsResetEnabled() const;
    SimulationSpeed GetSelectedSpeed() const;
    const std::string &GetPlayPauseText() const;

  protected:
    pipeframe::ui::View BuildNativeView() override;
private:
    pipeframe::ui::SimulationTransportModel model;
    std::string playPauseText{"PLAY"};
};
#endif
