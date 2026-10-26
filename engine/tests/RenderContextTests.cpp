#include <PipeFrame/Backend/SFML/RenderContextAdapter.h>
#include <PipeFrame/Render/CameraController2D.h>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Image.hpp>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void Require(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
static bool Near(pipeframe::Vector2f a, pipeframe::Vector2f b) {
    return std::abs(a.x-b.x)<.001f && std::abs(a.y-b.y)<.001f;
}
int main() {
    using namespace pipeframe;
    using namespace pipeframe::backend::sfml;
    try {
        bool rejected=false;
        try { RenderContext invalid(nullptr); } catch(const std::invalid_argument &) { rejected=true; }
        Require(rejected,"Null surface must be rejected");
        sf::RenderTexture texture({800,600});
        auto context=MakeRenderContext(texture);
        auto &camera=context.GetCamera();
        camera.SetSize({400,300});camera.SetCenter({20,30});
        camera.SetViewport({{.25f,.25f},{.5f,.5f}});
        Require(context.GetViewportRectangle()==Rectanglei{{200,150},{400,300}},"Normalized viewport");
        Require(Near(context.MapPixelToWorld({400,300}),{20,30}),"Viewport center mapping");
        Require(Near(context.MapPixelToWorld({200,150}),{-180,-120}),"Viewport origin mapping");
        Require(context.MapWorldToPixel({20,30})==Vector2i{400,300},"Inverse mapping");
        Require(context.IsInsideWorldViewport({200,150})&&!context.IsInsideWorldViewport({600,450}),"Half-open bounds");
        camera.SetZoom(2);camera.SetZoom(0);camera.SetZoom(-1);
        Require(Near(camera.GetSize(),{800,600}),"Zoom and invalid zoom preserve semantics");
        Require(Near(context.MapPixelToWorld({200,150}),{-380,-270}),"Zoom mapping");
        camera.Move({5,-5});Require(Near(camera.GetCenter(),{25,25}),"Camera move");
        context.BeginWorld();
        Require(texture.getView().getCenter()==sf::Vector2f{25,25},"World view selected");
        context.SetScreenSize({1000,700});context.BeginScreen();
        Require(texture.getView().getSize()==sf::Vector2f{1000,700},"Screen resize");
        Require(texture.getView().getViewport()==sf::FloatRect{{0,0},{1,1}},"Screen pass fills target");
        Require(texture.resize({1000,700}),"Target resize");
        Require(context.GetSize()==Vector2u{1000,700},"Live surface size");
        Require(context.GetViewportRectangle()==Rectanglei{{250,175},{500,350}},"Viewport after resize");
        context.SetScreenSize(context.GetSize());context.BeginScreen();
        texture.clear(sf::Color::Black);
        sf::RectangleShape shape({20,20});shape.setFillColor(sf::Color::Red);
        GetTarget(context).draw(shape);texture.display();
        Require(texture.getTexture().copyToImage().getPixel({10,10})==sf::Color::Red,"Offscreen submission");
        rejected=false;
        try { (void)GetWindow(context); } catch(const std::logic_error &) { rejected=true; }
        Require(rejected,"Offscreen window access must fail explicitly");
        CameraController2D controller;
        camera.SetCenter({0,0});camera.SetZoom(1);
        const auto anchor=context.MapPixelToWorld({500,350});
        controller.HandleEvent({InputEventType::PointerPressed,PointerInput{PointerButton::Middle,{500,350}}},context);
        controller.HandleEvent({InputEventType::PointerMoved,PointerMoveInput{{520,360}}},context);
        Require(Near(context.MapPixelToWorld({520,360}),anchor),"Middle pan preserves grab point");
        controller.HandleEvent({InputEventType::FocusChanged,FocusInput{false}},context);
        auto stopped=camera.GetCenter();
        controller.HandleEvent({InputEventType::PointerMoved,PointerMoveInput{{540,370}}},context);
        Require(Near(camera.GetCenter(),stopped),"Focus loss cancels camera drag");
        controller.HandleEvent({InputEventType::KeyPressed,KeyInput{InputKey::Space}},context);
        Require(controller.IsPanModifierActive(),"Neutral space press tracks modifier");
        controller.HandleEvent({InputEventType::PointerPressed,PointerInput{PointerButton::Left,{500,350}}},context);
        controller.HandleEvent({InputEventType::PointerMoved,PointerMoveInput{{530,360}}},context);
        Require(!Near(camera.GetCenter(),stopped),"Space-left pan works without native keyboard polling");
        controller.HandleEvent({InputEventType::PointerReleased,PointerInput{PointerButton::Left,{530,360}}},context);
        controller.HandleEvent({InputEventType::KeyReleased,KeyInput{InputKey::Space}},context);
        Require(!controller.IsPanModifierActive(),"Space release clears modifier");
        const auto zoomAnchor=context.MapPixelToWorld({700,400});
        controller.HandleEvent({InputEventType::WheelScrolled,WheelInput{1,{700,400},false}},context);
        Require(Near(context.MapPixelToWorld({700,400}),zoomAnchor),"Zoom preserves pointer world location");
        const auto zoom=camera.GetZoom();
        controller.HandleEvent({InputEventType::WheelScrolled,WheelInput{1,{700,400},true}},context);
        Require(camera.GetZoom()==zoom,"Horizontal scroll does not zoom");
        std::cout<<"Render context: viewport, mapping, zoom, resize, passes and offscreen rendering passed\n";
    } catch(const std::exception &error) { std::cerr<<error.what()<<'\n';return 1; }
}
