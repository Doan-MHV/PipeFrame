#ifndef PIPEFRAME_UI_TOAST_H
#define PIPEFRAME_UI_TOAST_H

#include <PipeFrame/Backend/SFML/UI/Surface.h>
#include <PipeFrame/Backend/SFML/UI/UITheme.h>

#include <functional>

class Toast final : public Surface {
public:
    using DismissedCallback = std::function<void()>;

    explicit Toast(const UITheme& theme = UITheme::Dark());

    void Show(float durationSeconds = 3.0f);
    void Dismiss();
    bool IsPresented() const;
    void SetOnDismissed(DismissedCallback callback);

protected:
    void OnUpdate(float realDeltaSeconds) override;

private:
    UITheme theme;
    DismissedCallback onDismissed;
    float remainingSeconds = 0.0f;
    bool presented = false;
    bool closing = false;
};

#endif
