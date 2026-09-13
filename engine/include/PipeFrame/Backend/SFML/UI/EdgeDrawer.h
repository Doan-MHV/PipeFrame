#ifndef PIPEFRAME_UI_EDGE_DRAWER_H
#define PIPEFRAME_UI_EDGE_DRAWER_H

#include <functional>

#include <PipeFrame/Backend/SFML/UI/Button.h>
#include <PipeFrame/Backend/SFML/UI/Surface.h>

#include <PipeFrame/UI/DrawerTypes.h>

class EdgeDrawer final : public Surface {
  public:
    using OpenChangedCallback = std::function<void(bool)>;

    explicit EdgeDrawer(DrawerEdge edge = DrawerEdge::Left,
                        const UITheme &theme = UITheme::Dark());

    void SetEdge(DrawerEdge edge);
    DrawerEdge GetEdge() const;

    void SetOpen(bool open, bool animate = true);
    bool IsOpen() const;
    void Open(bool animate = true);
    void Close(bool animate = true);
    void Toggle();

    void SetHandleExtent(float extent);
    float GetHandleExtent() const;
    void SetHandleLength(float length);
    float GetHandleLength() const;
    Button &GetHandle();
    const Button &GetHandle() const;

    void SetTransitionDuration(float seconds);
    void SetOnOpenChanged(OpenChangedCallback callback);

  protected:
    void OnGeometryChanged() override;
    void OnRender(sf::RenderTarget &target) const override;

  private:
    sf::Vector2f ClosedOffset() const;
    void RefreshHandleGeometry();
    void RefreshDrawerOffset(bool animate);

    Button &handle;
    UITheme theme;
    DrawerEdge edge;
    OpenChangedCallback onOpenChanged;
    float handleExtent = 28.0f;
    float handleLength = 96.0f;
    float transitionDuration = 0.16f;
    bool open = false;
};

#endif
