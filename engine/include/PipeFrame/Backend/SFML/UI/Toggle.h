#ifndef PIPEFRAME_UI_TOGGLE_H
#define PIPEFRAME_UI_TOGGLE_H

#include <PipeFrame/Backend/SFML/UI/Button.h>
#include <PipeFrame/Backend/SFML/UI/UITheme.h>

#include <functional>

class Toggle final : public Button {
public:
    using ChangedCallback = std::function<void(bool)>;

    explicit Toggle(const UITheme& theme = UITheme::Dark());

    void SetChecked(bool checked, bool notify = false);
    bool IsChecked() const;
    void SetOnChanged(ChangedCallback callback);

protected:
    void OnGeometryChanged() override;

private:
    void RefreshKnob(bool animate);

    Panel& knob;
    UITheme theme;
    ChangedCallback onChanged;
    bool checked = false;
};

#endif
