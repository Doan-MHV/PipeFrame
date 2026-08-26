#ifndef PIPEFRAME_VIEWPORT_TOOLBAR_H
#define PIPEFRAME_VIEWPORT_TOOLBAR_H

#include <functional>
#include <string>

#include <SFML/Graphics/Font.hpp>

#include <PipeFrame/UI/Panel.h>

class Label;
class TextButton;

class ViewportToolbar : public Panel {
  public:
    using ActionCallback = std::function<void()>;

    explicit ViewportToolbar(const sf::Font &font);

    void SetOnMetrics(ActionCallback callback);
    void SetOnUndo(ActionCallback callback);
    void SetOnRedo(ActionCallback callback);
    void SetOnSave(ActionCallback callback);
    void SetOnLoad(ActionCallback callback);

    void SetMetricsVisible(bool visible);
    void SetHistoryEnabled(bool undoEnabled, bool redoEnabled);
    void SetAuthoringEnabled(bool enabled);

    void SetStatusText(const std::string &text);

  protected:
    void OnGeometryChanged() override;

  private:
    TextButton *metricsButton = nullptr;
    TextButton *undoButton = nullptr;
    TextButton *redoButton = nullptr;
    TextButton *saveButton = nullptr;
    TextButton *loadButton = nullptr;

    Label *titleLabel = nullptr;
    Label *statusLabel = nullptr;
};

#endif