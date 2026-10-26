#pragma once
#include <PipeFrame/UI/View.h>

namespace pipeframe::ui {
// Declarative dialog content, hosted by a modal PopupLayer. The caller owns
// dismissal and action results; no backend types or application policy leak here.
inline View AlertDialog(std::string key, std::string title, View content,
                        std::vector<View> actions) {
    return views::Column(std::move(key), {
        views::Text("dialog-title", std::move(title)).FitHeight(),
        views::Scroll("dialog-content", std::move(content)).Expanded(),
        views::Wrap("dialog-actions", std::move(actions), 140).Spacing(8)
    }).Padding(16).Spacing(12).FillHeight();
}
} // namespace pipeframe::ui
