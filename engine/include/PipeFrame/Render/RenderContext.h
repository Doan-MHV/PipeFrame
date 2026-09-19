#pragma once
#include <PipeFrame/Render/Camera2D.h>
#include <PipeFrame/Render/Canvas.h>

#include <memory>
#include <stdexcept>

class RenderSurface {
public:
    virtual ~RenderSurface() = default;
    virtual pipeframe::Canvas GetCanvas() = 0;
    virtual pipeframe::Vector2u GetSize() const = 0;
    virtual void SetScreenSize(pipeframe::Vector2u) = 0;
    virtual void BeginWorld(const Camera2D&) = 0;
    virtual void BeginScreen() = 0;
    virtual pipeframe::Rectanglei Viewport(const Camera2D&) const = 0;
    virtual pipeframe::Vector2f PixelToWorld(pipeframe::Vector2i, const Camera2D&) const = 0;
    virtual pipeframe::Vector2i WorldToPixel(pipeframe::Vector2f, const Camera2D&) const = 0;
};

class RenderContext {
public:
    explicit RenderContext(std::shared_ptr<RenderSurface> value) : surface(std::move(value)) {
        if (!surface) throw std::invalid_argument("RenderContext requires a surface");
    }
    RenderSurface& GetSurface() const { return *surface; }
    pipeframe::Canvas GetCanvas() { return surface->GetCanvas(); }
    pipeframe::Vector2u GetSize() const { return surface->GetSize(); }
    Camera2D& GetCamera() { return camera; }
    const Camera2D& GetCamera() const { return camera; }
    pipeframe::Vector2f GetCameraCenter() const { return camera.GetCenter(); }
    pipeframe::Vector2f GetCameraSize() const { return camera.GetSize(); }
    void SetCameraCenter(pipeframe::Vector2f value) { camera.SetCenter(value); }
    void SetCameraSize(pipeframe::Vector2f value) { camera.SetSize(value); }
    void SetCameraZoom(float value) { camera.SetZoom(value); }
    pipeframe::Rectanglei GetViewportRectangle() const { return surface->Viewport(camera); }
    pipeframe::Vector2f MapPixelToWorld(pipeframe::Vector2i point) const {
        return surface->PixelToWorld(point, camera);
    }
    pipeframe::Vector2i MapWorldToPixel(pipeframe::Vector2f point) const {
        return surface->WorldToPixel(point, camera);
    }
    bool IsInsideWorldViewport(pipeframe::Vector2i point) const { return GetViewportRectangle().Contains(point); }
    void SetScreenSize(pipeframe::Vector2u size) { surface->SetScreenSize(size); }
    void BeginWorld() { surface->BeginWorld(camera); }
    void BeginScreen() { surface->BeginScreen(); }

private:
    std::shared_ptr<RenderSurface> surface;
    Camera2D camera;
};
