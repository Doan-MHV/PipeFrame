#include <PipeFrame/Backend/SFML/UI/Label.h>
#include <PipeFrame/Backend/SFML/UI/UIManager.h>

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <iostream>
#include <stdexcept>

#include "Editor/RoverMapPanel.h"

namespace {
void Check(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}
Widget* Find(Widget& root, const std::string& key) {
  if (root.GetKey() == key) return &root;
  for (std::size_t i = 0; i < root.GetChildCount(); ++i)
    if (auto* found = Find(*root.GetChild(i), key)) return found;
  return nullptr;
}
}  // namespace

int main() {
  try {
    using namespace pipeframe;
    using namespace RoverLab;
    BehaviourScene scene;
    ProjectWorld world(scene);
    ui::ProjectUIHost host;
    RoverMapPanel panel(world, host);
    auto rover = scene.CreateObject();
    rover.AddComponent<RoverMappingComponent>();
    auto sensor = scene.CreateObject();
    auto& mount = sensor.AddComponent<UltrasonicSensorComponent>();
    mount.parent.objectId = 2;
    mount.localPosition = {0.05f, 0.05f};
    world.simulation.mapping.SetObjectResolver(
      [&](SceneObjectId id) { return id == 2 ? rover : SceneObject{}; });
    // Deterministic corridor observations, using the same public events as simulation.
    const auto sample = [&](double time, double y) {
      RoverOdometryEvent pose;
      pose.rover = rover;
      pose.simulationTime = time;
      pose.x = 2;
      pose.y = y;
      pose.heading = std::numbers::pi / 2;
      world.simulation.events.Publish(pose);
      SensorReadingEvent reading;
      reading.sensor = sensor.GetEntity();
      reading.roverId = 2;
      reading.capturedAt = reading.simulationTime = time;
      reading.valid = true;
      reading.distance = 1;
      reading.status = SensorReadingStatus::Hit;
      world.simulation.events.Publish(reading);
    };
    for (int i = 0; i < 100; ++i)
      sample(i * 0.1, i * 0.1);
    UIManager manager;
    Check(manager.LoadDefaultFont(PIPEFRAME_TEST_FONT), "Font loads");
    auto mounted = manager.MountView(panel.DescribeView());
    mounted.SetBounds(0, 0, 380, 700);
    manager.Update(0);
    const auto refresh = [&] {
      panel.OnUpdate(0.1f);
      mounted.SetView(panel.DescribeView());
      manager.Update(0);
    };
    const auto widget = [&](const std::string& key) -> Widget& {
      auto* found = Find(*manager.GetRoot(0), key);
      Check(found, "Panel control exists");
      return *found;
    };
    const auto click = [&](const std::string& key) {
      const auto& control = widget(key);
      const auto p = control.GetScreenPosition() + control.GetSize() * 0.5f;
      manager.HandleEvent(
        sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {int(p.x), int(p.y)}});
      manager.HandleEvent(
        sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, {int(p.x), int(p.y)}});
      refresh();
    };
    const auto key = std::to_string(rover.GetEntity());
    auto& viewport = widget("map-" + key);
    const auto center = viewport.GetScreenPosition() + viewport.GetSize() * 0.5f;
    const sf::Vector2i point{int(center.x), int(center.y)};
    const auto text = [&]() -> std::string {
      return dynamic_cast<Label&>(widget("info-" + key)).GetText();
    };
    Check(text().find("Following rover") != std::string::npos, "Initial follow indicated");
    Check(world.simulation.mapping.Find(rover.GetEntity())->Index(2, 9.9) >= 0,
      "Fixture exceeds original bounds");
    manager.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, point});
    manager.HandleEvent(sf::Event::MouseMoved{{point.x + 40, point.y + 25}});
    manager.HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, point});
    refresh();
    Check(text().find("Manual view") != std::string::npos, "Real panel drag suspends follow");
    click("follow");
    Check(text().find("Following rover") != std::string::npos, "Follow button resumes tracking");
    click("in");
    Check(text().find("View: 8.00") == std::string::npos, "Zoom button changes scale");
    manager.HandleEvent(sf::Event::MouseWheelScrolled{sf::Mouse::Wheel::Vertical, -1, point});
    refresh();
    Check(text().find("Following rover") != std::string::npos, "Wheel zoom retains follow");
    sf::RenderTexture target({380, 700});
    const auto screenshot = [&](const char* path) {
      target.clear(sf::Color{20, 26, 34});
      manager.Render(target);
      target.display();
      Check(target.getTexture().copyToImage().saveToFile(path), "Save real panel render");
    };
    screenshot("/tmp/rover-minimap-follow.png");
    click("fit");
    Check(text().find("Manual view") != std::string::npos, "Fit switches to stable overview");
    screenshot("/tmp/rover-minimap-fit.png");
    world.ResetSensorState();
    sample(0, 0);
    refresh();
    Check(text().find("Following rover") != std::string::npos &&
            text().find("View: 8.00") != std::string::npos,
      "Reset resets actual panel camera");
    bool hidden{};
    panel.BindVisibility([&](bool visible) { hidden = !visible; });
    mounted.SetBounds(0, 0, 380, 400);
    refresh();
    const auto closePosition = widget("map-close").GetScreenPosition();
    manager.HandleEvent(sf::Event::MouseWheelScrolled{sf::Mouse::Wheel::Vertical, -20, {20, 180}});
    refresh();
    Check(widget("map-close").GetScreenPosition() == closePosition && closePosition.y < 60,
      "Close remains at the top when the panel content scrolls");
    click("map-close");
    Check(hidden, "Close delegates to the project UI host");
    std::cout << "Rover minimap rendering and input passed.\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
