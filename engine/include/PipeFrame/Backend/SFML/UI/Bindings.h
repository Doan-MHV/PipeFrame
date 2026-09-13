#ifndef PIPEFRAME_UI_BINDINGS_H
#define PIPEFRAME_UI_BINDINGS_H

#include <PipeFrame/Backend/SFML/UI/Label.h>
#include <PipeFrame/Backend/SFML/UI/ListView.h>
#include <PipeFrame/Backend/SFML/UI/Gauge.h>
#include <PipeFrame/Backend/SFML/UI/MetricCard.h>
#include <PipeFrame/Backend/SFML/UI/NumericField.h>
#include <PipeFrame/Backend/SFML/UI/ProgressBar.h>
#include <PipeFrame/Backend/SFML/UI/SegmentedControl.h>
#include <PipeFrame/Backend/SFML/UI/Slider.h>
#include <PipeFrame/UI/State.h>
#include <PipeFrame/Backend/SFML/UI/TabView.h>
#include <PipeFrame/Backend/SFML/UI/TextButton.h>
#include <PipeFrame/Backend/SFML/UI/Toggle.h>
#include <PipeFrame/Backend/SFML/UI/ZenModeShell.h>

#include <string>

namespace pipeframe::ui {

inline void BindText(Label &label, State<std::string> &state) {
    BindProperty(label, state,
                 [](Label &target, const std::string &text) { target.SetText(text); });
}

inline void BindText(TextButton &button, State<std::string> &state) {
    BindProperty(button, state,
                 [](TextButton &target, const std::string &text) { target.SetText(text); });
}

inline void BindValue(NumericField &field, State<float> &state) {
    BindProperty(field, state,
                 [](NumericField &target, const float value) { target.SetValue(value); });
}

inline void BindValue(Slider &slider, State<float> &state) {
    BindProperty(slider, state,
                 [](Slider &target, const float value) { target.SetValue(value); });
}

inline void BindValue(ProgressBar &progress, State<float> &state) {
    BindProperty(progress, state,
                 [](ProgressBar &target, const float value) { target.SetValue(value); });
}

inline void BindValue(Gauge &gauge, State<float> &state) {
    BindProperty(gauge, state,
                 [](Gauge &target, const float value) { target.SetValue(value); });
}

inline void BindValueText(MetricCard &card, State<std::string> &state) {
    BindProperty(card, state,
                 [](MetricCard &target, const std::string &value) {
                     target.SetValueText(value);
                 });
}

inline void BindChecked(Toggle &toggle, State<bool> &state) {
    BindProperty(toggle, state,
                 [](Toggle &target, const bool checked) { target.SetChecked(checked); });
}

inline void BindSelection(SegmentedControl &control, State<std::size_t> &state) {
    BindProperty(control, state,
                 [](SegmentedControl &target, const std::size_t index) {
                     target.SetSelectedIndex(index);
                 });
}

inline void BindSelection(TabView &tabs, State<std::size_t> &state) {
    BindProperty(tabs, state,
                 [](TabView &target, const std::size_t index) {
                     target.SetSelectedIndex(index);
                 });
}

inline void BindSelection(ListView &list, State<std::size_t> &state) {
    BindProperty(list, state,
                 [](ListView &target, const std::size_t index) {
                     target.SetSelectedIndex(index);
                 });
}

inline void BindZenMode(ZenModeShell &shell, State<bool> &state) {
    BindProperty(shell, state,
                 [](ZenModeShell &target, const bool enabled) {
                     target.SetZenMode(enabled);
                 });
}

} // namespace pipeframe::ui

#endif
