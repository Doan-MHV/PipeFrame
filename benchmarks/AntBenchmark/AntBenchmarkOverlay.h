#ifndef PIPEFRAME_ANT_BENCHMARK_OVERLAY_H
#define PIPEFRAME_ANT_BENCHMARK_OVERLAY_H

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>

struct AntBenchmarkMetrics {
    bool playing = false;
    bool renderingEnabled = true;

    std::size_t antCount = 0;
    std::uint64_t tickCount = 0;

    float framesPerSecond = 0.0f;
    float tickUpdateTimeMs = 0.0f;
    float renderGridTimeMs = 0.0f;
    float interactionGridTimeMs = 0.0f;
    float geometryTimeMs = 0.0f;
    float drawTimeMs = 0.0f;
    float zoom = 1.0f;

    std::string lodMode;

    std::size_t renderCandidateCount = 0;
    std::size_t visibleAntCount = 0;
    std::size_t vertexCount = 0;

    std::optional<std::uint32_t> selectedAntIndex;
    std::size_t pickCandidateCount = 0;
    std::size_t neighborCount = 0;
    std::size_t neighborCandidateCount = 0;

    bool separationEnabled = false;
    float behaviorTimeMs = 0.0f;

    std::size_t behaviorProcessedAntCount = 0;
    std::uint64_t behaviorCandidateCheckCount = 0;
    std::uint64_t behaviorNeighborInteractionCount = 0;

    std::size_t behaviorSliceCount = 1;
    float behaviorAgentUpdateRateHz = 60.0f;
    
    float ticksPerSecond = 0.0f;
};

class AntBenchmarkOverlay {
  public:
    AntBenchmarkOverlay();

    bool Load(const std::filesystem::path &fontPath);

    void ToggleVisible();

    void Update(const AntBenchmarkMetrics &metrics);

    void Render(sf::RenderTarget &target) const;

  private:
    sf::Font font;
    sf::Text text;
    sf::RectangleShape background;

    bool visible = true;
};

#endif