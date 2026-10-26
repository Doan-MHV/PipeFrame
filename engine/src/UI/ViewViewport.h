#pragma once

#include <PipeFrame/Backend/SFML/UI/Widget.h>
#include <PipeFrame/Backend/SFML/VertexAccess.h>
#include <PipeFrame/UI/View.h>

#include <SFML/Graphics/RectangleShape.hpp>

namespace pipeframe::ui {
class ViewViewport final : public Widget {
public:
  std::vector<View::MeshLayer> layers;
  Viewport2D camera;
  Color background{30, 35, 43};
  bool zoomAtPointer{true};
  std::function<void(const Viewport2D&, bool)> onChanged;
  std::function<void(Vector2f)> onResized;
  std::function<void(Vector2f)> onClicked;

  ViewViewport() { SetFocusable(true); }

protected:
  void OnGeometryChanged() override {
    const auto size = GetSize();
    if (size == previousSize) return;
    previousSize = size;
    const auto callback = onResized;
    if (callback) callback({size.x, size.y});
  }
  void OnRender(sf::RenderTarget& target) const override {
    const auto pixels = target.getSize();
    const float extent = std::min(GetSize().x, GetSize().y);
    if (!pixels.x || !pixels.y || extent <= 0) return;
    const auto previous = target.getView();
    const auto start = target.mapCoordsToPixel(GetScreenPosition());
    const auto end = target.mapCoordsToPixel(GetScreenPosition() + GetSize());
    const sf::FloatRect clip{{float(start.x) / pixels.x, float(start.y) / pixels.y},
      {float(std::max(0, end.x - start.x)) / pixels.x,
        float(std::max(0, end.y - start.y)) / pixels.y}};
    auto view = previous;
    view.setScissor(previous.getScissor().findIntersection(clip).value_or(sf::FloatRect{}));
    target.setView(view);
    sf::RectangleShape backdrop(GetSize());
    backdrop.setPosition(GetScreenPosition());
    backdrop.setFillColor({background.r, background.g, background.b, background.a});
    target.draw(backdrop);
    sf::RenderStates states;
    states.transform.translate(GetScreenPosition() + GetSize() * 0.5f);
    states.transform.scale({extent / camera.span, extent / camera.span});
    states.transform.translate({-camera.center.x, -camera.center.y});
    for (const auto& layer : layers)
      backend::sfml::DrawVertices(target, layer.vertices, sf::PrimitiveType::Triangles, states);
    target.setView(previous);
  }

  bool OnEvent(const sf::Event& event) override {
    if (const auto* press = event.getIf<sf::Event::MouseButtonPressed>()) {
      if (press->button != sf::Mouse::Button::Left && press->button != sf::Mouse::Button::Middle)
        return false;
      dragging = true;
      dragButton = press->button;
      last = pressed = sf::Vector2f(press->position);
      moved = false;
      return true;
    }
    if (const auto* release = event.getIf<sf::Event::MouseButtonReleased>()) {
      if (release->button == dragButton && dragging) {
        dragging = false;
        const auto position = sf::Vector2f(release->position);
        const auto local = position - GetScreenPosition();
        const auto delta = position - pressed;
        const float extent = std::min(GetSize().x, GetSize().y);
        if (!moved && delta.x * delta.x + delta.y * delta.y <= 16 &&
            dragButton == sf::Mouse::Button::Left && extent > 0 && local.x >= 0 && local.y >= 0 &&
            local.x < GetSize().x && local.y < GetSize().y) {
          const auto offset = (local - GetSize() * .5f) / extent;
          const auto point = camera.WorldAt({offset.x, offset.y});
          const auto callback = onClicked;
          if (callback) callback(point);
        }
      }
      return true;
    }
    const float extent = std::min(GetSize().x, GetSize().y);
    if (const auto* move = event.getIf<sf::Event::MouseMoved>(); move && dragging && extent > 0) {
      const auto position = sf::Vector2f(move->position);
      const auto total = position - pressed;
      if (!moved && total.x * total.x + total.y * total.y <= 16) return true;
      moved = true;
      const auto delta = (position - last) / extent;
      last = position;
      if (delta.x || delta.y) {
        camera.Pan({delta.x, delta.y});
        Notify(true);
      }
      return true;
    }
    if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>(); wheel && extent > 0) {
      const auto offset =
        (sf::Vector2f(wheel->position) - GetScreenPosition() - GetSize() * 0.5f) / extent;
      camera.Zoom(wheel->delta, zoomAtPointer ? Vector2f{offset.x, offset.y} : Vector2f{});
      Notify(false);
      return true;  // Never scroll the enclosing panel when zooming the map.
    }
    return false;
  }
  void OnKeyboardFocusLost() override { dragging = false; }
  void OnEnabledChanged() override {
    if (!IsEnabled()) dragging = false;
  }

private:
  void Notify(bool panned) {
    // Call a copy: a consumer may replace its description from the callback.
    const auto callback = onChanged;
    const auto value = camera;
    if (callback) callback(value, panned);
  }
  bool dragging{}, moved{};
  sf::Mouse::Button dragButton{sf::Mouse::Button::Left};
  sf::Vector2f last{}, pressed{}, previousSize{};
};
}  // namespace pipeframe::ui
