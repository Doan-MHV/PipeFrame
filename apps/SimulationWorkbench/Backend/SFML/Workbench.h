#ifndef PIPEFRAME_WORKBENCH_H
#define PIPEFRAME_WORKBENCH_H

#include <filesystem>
#include <memory>
#include <optional>

#include <PipeFrame/Backend/SFML/Core/Scene.h>

class Workbench final : public Scene {
public:
    explicit Workbench(
        std::optional<std::filesystem::path>
            startupProject = std::nullopt);

    ~Workbench() override;

    Workbench(const Workbench &) = delete;
    Workbench &operator=(const Workbench &) = delete;

    void Load() override;
    void Start() override;

    void OnResize(
        sf::Vector2u newSize,
        RenderContext &context) override;

    void HandleEvent(
        const sf::Event &event,
        RenderContext &context) override;

    void FixedUpdate(float fixedDeltaTime) override;
    void Update(float deltaTime) override;

    void Render(RenderContext &context) override;
    unsigned int GetFrameRateLimit() const override;
    float GetSimulationTimeScale() const override;
    bool UseMaximumSimulationRate() const override;

    void Stop() override;
    void Unload() override;

private:
    class Implementation;

    std::unique_ptr<Implementation> implementation;
};

std::unique_ptr<Scene> CreateWorkbench(
    std::optional<std::filesystem::path>
        startupProject = std::nullopt);

#endif
