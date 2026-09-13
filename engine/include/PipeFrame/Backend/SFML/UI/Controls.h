#ifndef PIPEFRAME_UI_CONTROLS_H
#define PIPEFRAME_UI_CONTROLS_H

#include <PipeFrame/Backend/SFML/SimulationDashboardHost.h>
#include <PipeFrame/Backend/SFML/UI/Slider.h>
#include <initializer_list>

namespace pipeframe::ui {
struct Action {
    std::string text;
    std::function<void()> onPressed;
};
struct Range {
    float minimum{}, maximum{1}, value{}, step{};
    std::function<void(float)> onChanged;
};

// A themed construction context: projects describe controls and callbacks.
// PipeFrame owns child lifetime, default dimensions, spacing and equal flex.
class Controls {
public:
    explicit Controls(NativeSimulationDashboard &dashboard) : dashboard(dashboard) {}

    TextButton &Button(Widget &parent, Action action) const {
        return dashboard.Action(parent, action.text, std::move(action.onPressed));
    }
    std::vector<TextButton *> ActionRow(Widget &parent, std::initializer_list<Action> actions) const {
        auto &row = parent.CreateChild<Row>();
        SimulationDashboard::Clear(row);
        row.SetSize({0, 34});
        row.SetSpacing(6);
        std::vector<TextButton *> result;
        for (const auto &action : actions) {
            auto &button = Button(row, action);
            row.SetChildFlex(button, 1);
            result.push_back(&button);
        }
        return result;
    }
    Slider &RangeInput(Widget &parent, Range range) const {
        auto &slider = parent.CreateChild<Slider>();
        slider.SetSize({0, 32});
        slider.SetRange(range.minimum, range.maximum);
        slider.SetStep(range.step);
        slider.SetValue(range.value);
        slider.SetOnValueChanged(std::move(range.onChanged));
        return slider;
    }
private:
    NativeSimulationDashboard &dashboard;
};
} // namespace pipeframe::ui
#endif
