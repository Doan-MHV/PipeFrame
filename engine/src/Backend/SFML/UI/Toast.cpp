#include <PipeFrame/Backend/SFML/UI/Toast.h>

#include <algorithm>
#include <utility>

Toast::Toast(const UITheme &newTheme) : Surface(SurfaceVariant::Floating, newTheme), theme(newTheme) {
    SetSize({280.0f, 52.0f});
    SetHitTestVisible(false);
    SetVisible(false);
}

void Toast::Show(const float durationSeconds) {
    remainingSeconds = std::max(0.0f, durationSeconds);
    presented = true;
    closing = false;
    SetVisible(true);
    SetOpacity(0.0f);
    SetVisualOffset({0.0f, theme.spacing8});
    AnimateOpacityTo(1.0f, theme.motionFast);
    AnimateVisualOffsetTo({0.0f, 0.0f}, theme.motionNormal);
}

void Toast::Dismiss() {
    if (!presented || closing) {
        return;
    }
    closing = true;
    remainingSeconds = 0.0f;
    AnimateOpacityTo(0.0f, theme.motionFast);
    AnimateVisualOffsetTo({0.0f, -theme.spacing4}, theme.motionFast);
    if (IsReducedMotion()) {
        OnUpdate(0.0f);
    }
}

bool Toast::IsPresented() const { return presented && !closing; }

void Toast::SetOnDismissed(DismissedCallback callback) { onDismissed = std::move(callback); }

void Toast::OnUpdate(const float realDeltaSeconds) {
    if (!closing) {
        if (remainingSeconds > 0.0f) {
            remainingSeconds = std::max(0.0f, remainingSeconds - realDeltaSeconds);
            if (remainingSeconds == 0.0f) {
                Dismiss();
            }
        }
        return;
    }
    if (IsMotionActive()) {
        return;
    }
    closing = false;
    presented = false;
    SetVisible(false);
    if (onDismissed) {
        onDismissed();
    }
}
