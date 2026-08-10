#include "PipeFrame/Render/Camera2D.h"

Camera2D::Camera2D()
{
    RefreshView();
}

void Camera2D::SetCenter(sf::Vector2f newCenter)
{
    center = newCenter;
    RefreshView();
}

void Camera2D::Move(sf::Vector2f offset)
{
    center += offset;
    RefreshView();
}

void Camera2D::SetSize(sf::Vector2f size)
{
    baseSize = size;
    RefreshView();
}

void Camera2D::SetZoom(float newZoom)
{
    if (newZoom <= 0.0f)
    {
        return;
    }

    zoom = newZoom;
    RefreshView();
}

sf::Vector2f Camera2D::GetCenter() const
{
    return center;
}

sf::Vector2f Camera2D::GetSize() const
{
    return baseSize * zoom;
}

float Camera2D::GetZoom() const
{
    return zoom;
}

const sf::View& Camera2D::GetView() const
{
    return view;
}

void Camera2D::RefreshView()
{
    view.setCenter(center);
    view.setSize(baseSize * zoom);
}
