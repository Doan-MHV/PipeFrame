#pragma once
#include <PipeFrame/Simulation/SimulationController.h>
#include <PipeFrame/UI/View.h>
namespace pipeframe::ui {
struct SimulationTransportModel {
    bool playing{}, previewActive{};
    SimulationSpeed speed{SimulationSpeed::Realtime};
    std::function<void()> playPause, step, reset;
    std::function<void(SimulationSpeed)> selectSpeed;
};
inline View SimulationTransportView(std::string key, const SimulationTransportModel& model,
                                    bool compact = false) {
    std::vector<View> speeds;
    const char* names[]{"1X", "2X", "4X", "MAX"};
    for (int i = 0; i < 4; ++i)
        speeds.push_back(views::Button("speed:" + std::to_string(i), names[i], [callback = model.selectSpeed, i] {
                             if (callback) callback(static_cast<SimulationSpeed>(i));
                         }).Selected(static_cast<int>(model.speed) == i));
    auto actions = views::Row(
        "actions",
        {views::Button("play", model.playing ? "PAUSE" : "PLAY", model.playPause),
         views::Button("step", "STEP", model.step).Enabled(!model.playing),
         views::Button("reset", "RESET", model.reset).Enabled(!model.playing && model.previewActive)});
    auto speedRow = views::Row("speeds", std::move(speeds));
    if (compact) {
        for (auto& button : actions.children) button.Width(80).Height(32);
        for (auto& button : speedRow.children) button.Width(48).Height(32);
        actions.Width(248).Spacing(4);
        speedRow.Width(204).Spacing(4);
        return views::Row(std::move(key), {
            views::Text("left-space", "").Expanded(),
            std::move(actions), std::move(speedRow),
            views::Text("right-space", "").Expanded()
        }).Padding(6).Spacing(12);
    }
    return views::Column(std::move(key), {
        views::Text("label", "SIMULATION").FitHeight(),
        std::move(actions), std::move(speedRow)
    }).Padding(8).Surface();
}
}  // namespace pipeframe::ui
