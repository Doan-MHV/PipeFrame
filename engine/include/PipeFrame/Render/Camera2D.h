#pragma once
#include <PipeFrame/Foundation/MathTypes.h>

class Camera2D {
public:
    void SetCenter(pipeframe::Vector2f value) { center = value; }
    void Move(pipeframe::Vector2f offset) { center += offset; }
    void SetSize(pipeframe::Vector2f value) { baseSize = value; }
    void SetZoom(float value) { if (value > 0) zoom = value; }
    pipeframe::Vector2f GetCenter() const { return center; }
    pipeframe::Vector2f GetSize() const { return baseSize * zoom; }
    float GetZoom() const { return zoom; }
    void SetViewport(pipeframe::Rectanglef value) { viewport = value; }
    pipeframe::Rectanglef GetViewport() const { return viewport; }
private:
    pipeframe::Vector2f center{}, baseSize{1280,720};
    pipeframe::Rectanglef viewport{{0,0},{1,1}};
    float zoom = 1;
};
