#ifndef PIPEFRAME_CAMERA2D_H
#define PIPEFRAME_CAMERA2D_H

#include <SFML/Graphics/View.hpp>
#include <SFML/System/Vector2.hpp>

class Camera2D
{
public:
    Camera2D();

    void SetCenter(sf::Vector2f center);
    void Move(sf::Vector2f offset);

    void SetSize(sf::Vector2f size);
    void SetZoom(float zoom);

    sf::Vector2f GetCenter() const;
    sf::Vector2f GetSize() const;
    float GetZoom() const;

    const sf::View& GetView() const;

private:
    void RefreshView();

    sf::View view;

    sf::Vector2f center{0.0f, 0.0f};
    sf::Vector2f baseSize{1280.0f, 720.0f};
    float zoom = 1.0f;
};

#endif //PIPEFRAME_CAMERA2D_H
