#include <PipeFrame/Backend/SFML/RenderContextAdapter.h>
#include <PipeFrame/Backend/SFML/CanvasAdapter.h>
namespace pipeframe::backend::sfml {
namespace {
sf::View NativeView(const Camera2D &camera) {
    sf::View view;
    const auto center=camera.GetCenter(), size=camera.GetSize();
    const auto viewport=camera.GetViewport();
    view.setCenter({center.x,center.y}); view.setSize({size.x,size.y});
    view.setViewport({{viewport.position.x,viewport.position.y},{viewport.size.x,viewport.size.y}});
    return view;
}
class Surface final : public RenderSurface {
public:
    Surface(sf::RenderTarget &value, sf::RenderWindow *host=nullptr) : target(value), window(host) {
        const auto size=target.getSize(); SetScreenSize({size.x,size.y});
    }
    Canvas GetCanvas() override { return MakeCanvas(target); }
    Vector2u GetSize() const override { const auto v=target.getSize(); return {v.x,v.y}; }
    void SetScreenSize(Vector2u size) override {
        screen.setSize({float(size.x),float(size.y)}); screen.setCenter({size.x*.5f,size.y*.5f});
    }
    void BeginWorld(const Camera2D &camera) override { target.setView(NativeView(camera)); }
    void BeginScreen() override { target.setView(screen); }
    Rectanglei Viewport(const Camera2D &camera) const override {
        const auto r=target.getViewport(NativeView(camera));return {{r.position.x,r.position.y},{r.size.x,r.size.y}};
    }
    Vector2f PixelToWorld(Vector2i p,const Camera2D &camera) const override {
        const auto v=target.mapPixelToCoords({p.x,p.y},NativeView(camera));return {v.x,v.y};
    }
    Vector2i WorldToPixel(Vector2f p,const Camera2D &camera) const override {
        const auto v=target.mapCoordsToPixel({p.x,p.y},NativeView(camera));return {v.x,v.y};
    }
    sf::RenderTarget &target;
    sf::RenderWindow *window;
    sf::View screen;
};
}
RenderContext MakeRenderContext(sf::RenderWindow &window) { return RenderContext(std::make_shared<Surface>(window,&window)); }
RenderContext MakeRenderContext(sf::RenderTexture &texture) { return RenderContext(std::make_shared<Surface>(texture)); }
sf::RenderTarget &GetTarget(const RenderContext &context) { return dynamic_cast<Surface &>(context.GetSurface()).target; }
sf::RenderWindow &GetWindow(const RenderContext &context) {
    auto *window=dynamic_cast<Surface &>(context.GetSurface()).window;
    if (!window) throw std::logic_error("Offscreen surface has no window");
    return *window;
}
}
