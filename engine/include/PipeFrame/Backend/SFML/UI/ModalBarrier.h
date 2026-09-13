#ifndef PIPEFRAME_UI_MODAL_BARRIER_H
#define PIPEFRAME_UI_MODAL_BARRIER_H

#include <functional>

#include <PipeFrame/Backend/SFML/UI/Panel.h>
#include <PipeFrame/Backend/SFML/UI/UITheme.h>

class ModalBarrier final : public Panel {
  public:
    using DismissCallback = std::function<void()>;

    explicit ModalBarrier(const UITheme &theme = UITheme::Dark());

    void SetDismissOnBackgroundClick(bool dismiss);
    bool DismissesOnBackgroundClick() const;
    void SetOnDismiss(DismissCallback callback);

  protected:
    bool OnEvent(const sf::Event &event) override;

  private:
    DismissCallback onDismiss;
    bool dismissOnBackgroundClick = true;
    bool pointerPressed = false;
};

#endif
