#ifndef SAILBOAT_RENDERER_H
#define SAILBOAT_RENDERER_H

#include <array>
#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <PipeFrame/Resources/GraphicsResourceService.h>

#include "Components/Boat.h"
#include "Training/SailBoatAgent.h"
#include "Training/SailBoatRaceTask.h"
#include "World/RaceCourse.h"

namespace sailboat_simulation {

struct SailBoatRenderStatistics final {
    std::size_t candidateBoats{0};
    std::size_t visibleBoats{0};
    std::size_t culledBoats{0};
    std::size_t ghostBoats{0};
    std::size_t boatVertices{0};
    std::size_t trajectoryVertices{0};
    std::size_t wakeSources{0};
    bool waterShaderActive{false};
    std::size_t populationDrawCalls{0};
};

class SailBoatRenderer final {
public:
    bool LoadAssets(const std::filesystem::path &assetRoot, std::string &errorMessage);
    void UnloadAssets();
    void Advance(float deltaTime);
    void SetRaceProgress(std::size_t reachedTargetCount);
    void ResetRaceProgress();
    void ResetWaterSimulation();

    void DrawWater(sf::RenderTarget &target, sf::Vector2f worldSize,
                   sf::Vector2f wind, bool animateWater,
                   std::span<const SailBoatAgent> agents = {},
                   std::size_t bestAgentIndex = 0,
                   bool onlyBestCreatesWake = true,
                   bool advanceSimulation = true);
    void DrawPopulation(sf::RenderTarget &target, std::span<const SailBoatAgent> agents,
                        std::size_t bestAgentIndex, bool drawGhosts = true,
                        bool highlightBest = true);
    void DrawBoat(sf::RenderTarget &target, const Boat &boat,
                  sf::Color color = sf::Color::White) const;
    void DrawTrajectory(sf::RenderTarget &target, const Boat &boat);
    void DrawCourse(sf::RenderTarget &target, const RaceCourse &course,
                    const SailBoatRaceTask &bestTask, bool drawLabels = true) const;
    void DrawNextTarget(sf::RenderTarget &target, const Boat &boat,
                        const SailBoatRaceTask &task, const RaceCourse &course) const;

    [[nodiscard]] bool AreAssetsLoaded() const;
    [[nodiscard]] float GetWaterTime() const;
    [[nodiscard]] const SailBoatRenderStatistics &GetStatistics() const;

private:
    bool InitializeWaterSimulation(sf::Vector2f worldSize);
    void DrawAnimatedWater(sf::RenderTarget &target, sf::Vector2f worldSize,
                           std::span<const SailBoatAgent> agents,
                           std::size_t bestAgentIndex,
                           bool onlyBestCreatesWake,
                           bool advanceSimulation);
    void DrawWaterPlaceholder(sf::RenderTarget &target, sf::Vector2f worldSize);
    static void WriteBoatQuad(std::vector<sf::Vertex> &vertices,std::size_t quadIndex,
                              const Boat &boat, sf::Color color);
    void DrawMark(sf::RenderTarget &target, sf::Vector2f position, std::size_t label,
                  sf::Color outlineColor, float scale) const;

    pipeframe::GraphicsResourceService resources;
    pipeframe::TextureHandle boatTexture,boatDepthTexture,waterTexture,dottedTexture,checkTexture;
    pipeframe::FontHandle font;
    pipeframe::ShaderHandle waterSimulationShader,waterRenderShader;
    std::array<pipeframe::RenderSurfaceHandle,2> waterHeightTextures{};
    std::vector<sf::Vertex> wakeVertices,boatVertices,trajectoryVertices;
    SailBoatRenderStatistics statistics;
    float waterTime{0.0f};
    float reachedMarkPulse{0.0f};
    std::size_t reachedTargetCount{0};
    std::size_t waterReadTexture{0};
    sf::Vector2f waterWorldSize{};
    sf::Vector2u waterSimulationSize{};
    bool assetsLoaded{false};
    bool waterShaderLoaded{false};
    bool waterSimulationInitialized{false};
};

} // namespace sailboat_simulation

#endif
