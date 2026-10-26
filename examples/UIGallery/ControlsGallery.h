#pragma once

#include <functional>
#include <memory>
#include <string>
#include <PipeFrame/Backend/SFML/UI/UIManager.h>

class ControlsGallery {
  public:
    explicit ControlsGallery(const sf::Font &font);
    ~ControlsGallery();
    void Resize(sf::Vector2u size);
    void Update(float delta);
    void HandleEvent(const sf::Event &event);
    void Render(sf::RenderTarget &target);
    void SetOnMonitoring(std::function<void()> callback);
    void Present(const std::string &scene);
    bool CheckInteractions();
  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
