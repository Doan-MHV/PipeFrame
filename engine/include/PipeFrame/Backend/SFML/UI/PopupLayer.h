#ifndef PIPEFRAME_UI_POPUP_LAYER_H
#define PIPEFRAME_UI_POPUP_LAYER_H

#include <functional>

#include <PipeFrame/Backend/SFML/UI/Surface.h>
#include <PipeFrame/Backend/SFML/UI/UITheme.h>

class PopupLayer final : public Panel {
  public:
    using DismissCallback = std::function<void()>;

    explicit PopupLayer(const UITheme &theme = UITheme::Dark());

    Surface &GetContent();
    const Surface &GetContent() const;

    void SetContentBounds(const sf::FloatRect &bounds);
    sf::FloatRect GetContentBounds() const;

    void SetDismissOnBackgroundClick(bool dismiss);
    bool DismissesOnBackgroundClick() const;
    void SetOnDismiss(DismissCallback callback);

    void Open();
    void Dismiss();
    bool IsOpen() const;

  protected:
    bool OnEvent(const sf::Event &event) override;
    void OnGeometryChanged() override;
    void OnUpdate(float realDeltaSeconds) override;

  private:
    void ArrangeContent();

    Surface &content;
    UITheme theme;
    DismissCallback onDismiss;
    sf::FloatRect requestedContentBounds{{0.0f, 0.0f}, {320.0f, 180.0f}};
    bool dismissOnBackgroundClick = true;
    bool backgroundPointerPressed = false;
    bool open = false;
    bool closing = false;
};

#endif
