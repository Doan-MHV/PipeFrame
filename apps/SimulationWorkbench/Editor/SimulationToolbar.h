#pragma once
#include <PipeFrame/UI/SimulationTransportView.h>
#include <PipeFrame/UI/ViewPanel.h>

// Global editor controls, independent of inspector selection and scrolling.
class SimulationToolbar : public pipeframe::ui::ViewPanel {
public:
    SimulationToolbar() { SetPreferredSize({600, 44}); }
    void SetOnPlayPause(std::function<void()> callback) { model.playPause=std::move(callback); RefreshView(); }
    void SetOnSingleStep(std::function<void()> callback) { model.step=std::move(callback); RefreshView(); }
    void SetOnReset(std::function<void()> callback) { model.reset=std::move(callback); RefreshView(); }
    void SetOnSpeedSelected(std::function<void(SimulationSpeed)> callback) { model.selectSpeed=std::move(callback); RefreshView(); }
    void SetSimulationState(bool playing, bool previewActive, SimulationSpeed speed) {
        if (model.playing==playing && model.previewActive==previewActive && model.speed==speed) return;
        model.playing=playing; model.previewActive=previewActive; model.speed=speed; RefreshView();
    }
protected:
    pipeframe::ui::View BuildView() override {
        return pipeframe::ui::SimulationTransportView("simulation-toolbar", model, true);
    }
private:
    pipeframe::ui::SimulationTransportModel model;
};
