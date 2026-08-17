#include "AntBenchmarkOverlay.h"

#include <iomanip>
#include <sstream>

AntBenchmarkOverlay::AntBenchmarkOverlay() : text(font, "", 12) {
    background.setPosition({12.0f, 12.0f});
    background.setSize({390.0f, 100.0f});
    background.setFillColor(sf::Color(18, 20, 26, 225));
    background.setOutlineColor(sf::Color(75, 82, 98));
    background.setOutlineThickness(1.0f);

    text.setPosition({24.0f, 22.0f});
    text.setFillColor(sf::Color(225, 230, 240));
    text.setLineSpacing(1.1f);
}

bool AntBenchmarkOverlay::Load(const std::filesystem::path &fontPath) { return font.openFromFile(fontPath); }

void AntBenchmarkOverlay::ToggleVisible() { visible = !visible; }

void AntBenchmarkOverlay::Update(const AntBenchmarkMetrics &metrics) {
    std::ostringstream stream;

    stream << std::fixed << std::setprecision(2);

    stream << "SIMULATION\n"
           << (metrics.playing ? "PLAYING" : "PAUSED") << '\n'
           << "Ants:             " << metrics.antCount << '\n'
           << "FPS:              " << metrics.framesPerSecond << '\n'
           << "Tick update:      " << metrics.tickUpdateTimeMs << " ms\n"
           << "Render grid:      " << metrics.renderGridTimeMs << " ms\n"
           << "Interaction grid: " << metrics.interactionGridTimeMs << " ms\n"
           << "Tick rate:        " << metrics.ticksPerSecond << " Hz\n\n"

           << "RENDERING\n"
           << "Enabled:          " << (metrics.renderingEnabled ? "YES" : "NO") << '\n'
           << "LOD:              " << metrics.lodMode << '\n'
           << "Zoom:             " << metrics.zoom << '\n'
           << "Candidates:       " << metrics.renderCandidateCount << '\n'
           << "Visible:          " << metrics.visibleAntCount << '\n'
           << "Vertices:         " << metrics.vertexCount << '\n'
           << "Geometry:         " << metrics.geometryTimeMs << " ms\n"
           << "Draw:             " << metrics.drawTimeMs << " ms\n\n"

           << "SELECTION\n"
           << "Ant:              ";

    if (metrics.selectedAntIndex) {
        stream << *metrics.selectedAntIndex;
    } else {
        stream << "NONE";
    }

    stream << '\n'
           << "Pick candidates:  " << metrics.pickCandidateCount << '\n'
           << "Neighbors:        " << metrics.neighborCount << '/' << metrics.neighborCandidateCount;

    stream << "\n\nBEHAVIOR\n"
           << "Separation:       " << (metrics.separationEnabled ? "ON" : "OFF") << '\n'
           << "Behavior time:    " << metrics.behaviorTimeMs << " ms\n"
           << "Processed:        " << metrics.behaviorProcessedAntCount << '\n'
           << "Candidate checks: " << metrics.behaviorCandidateCheckCount << '\n'
           << "Interactions:     " << metrics.behaviorNeighborInteractionCount << '\n'
           << "Slices:           " << metrics.behaviorSliceCount << '\n'
           << "Agent rate:       " << metrics.behaviorAgentUpdateRateHz << " Hz\n"

           << "\nCONTROLS\n"
           << "1: 10K   2: 100K   3: 250K   4: 1M\n"
           << "B: separation     N: behavior slices\n"
           << "A: automatic LOD  L: points/quads\n"
           << "R: rendering      P: pause\n"
           << ".: single step    H: hide overlay\n";

    text.setString(stream.str());

    constexpr float MinimumPanelWidth = 390.0f;
    constexpr float PanelPadding = 12.0f;

    const sf::FloatRect textBounds = text.getGlobalBounds();

    const sf::Vector2f panelPosition = background.getPosition();

    const float contentRight = textBounds.position.x + textBounds.size.x;

    const float contentBottom = textBounds.position.y + textBounds.size.y;

    background.setSize({std::max(MinimumPanelWidth, contentRight - panelPosition.x + PanelPadding),
                        contentBottom - panelPosition.y + PanelPadding});
}

void AntBenchmarkOverlay::Render(sf::RenderTarget &target) const {
    if (!visible) {
        return;
    }

    target.draw(background);
    target.draw(text);
}