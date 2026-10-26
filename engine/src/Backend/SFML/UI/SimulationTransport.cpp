#include <PipeFrame/Backend/SFML/UI/SimulationTransport.h>
SimulationTransport::SimulationTransport(const sf::Font &font) : NativeViewPanel(font) {
    SetSize({0, 128});
    SetOutlineThickness(0);
    SetFillColor(sf::Color::Transparent);
}
void SimulationTransport::SetOnPlayPause(ActionCallback cb) {
    model.playPause = std::move(cb);
    RefreshView();
}
void SimulationTransport::SetOnSingleStep(ActionCallback cb) {
    model.step = std::move(cb);
    RefreshView();
}
void SimulationTransport::SetOnReset(ActionCallback cb) {
    model.reset = std::move(cb);
    RefreshView();
}
void SimulationTransport::SetOnSpeedSelected(SpeedSelectedCallback cb) {
    model.selectSpeed = std::move(cb);
    RefreshView();
}
void SimulationTransport::SetSimulationState(bool playing, bool active, SimulationSpeed speed) {
    if (model.playing == playing && model.previewActive == active && model.speed == speed)
        return;
    model.playing = playing;
    model.previewActive = active;
    model.speed = speed;
    playPauseText = playing ? "PAUSE" : "PLAY";
    RefreshView();
}
bool SimulationTransport::IsStepEnabled() const { return !model.playing; }
bool SimulationTransport::IsResetEnabled() const { return !model.playing && model.previewActive; }
SimulationSpeed SimulationTransport::GetSelectedSpeed() const { return model.speed; }
const std::string &SimulationTransport::GetPlayPauseText() const { return playPauseText; }
pipeframe::ui::View SimulationTransport::BuildNativeView() {
    return pipeframe::ui::SimulationTransportView("transport", model);
}
