#include "SailBoatRenderer.h"
#include <PipeFrame/Backend/SFML/GraphicsResourceAccess.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/CircleShape.hpp>

namespace sailboat_simulation {
namespace {

constexpr sf::Vector2f BoatTextureSize{379.0f, 103.0f};
constexpr float BoatWorldLength = 12.0f;
constexpr float BoatWorldWidth = BoatWorldLength * BoatTextureSize.y / BoatTextureSize.x;
constexpr float RouteWidth = 3.2f;
constexpr sf::Color SeaColor{33, 150, 243, 180};
constexpr sf::Color BestColor{255, 209, 102};
constexpr sf::Color CrashedColor{242, 97, 87};
constexpr float MarkRadius = 5.0f;
// Pezza runs the height field at four pixels per world unit (1024 -> 4096).
// Keep the same spatial density where possible, while retaining a practical GPU cap
// for PipeFrame courses that are larger than Pezza's 1024-unit environment.
constexpr float PezzaWaterPixelsPerWorldUnit = 4.0f;
constexpr unsigned int MaximumWaterTextureDimension = 4096;
constexpr float SeaTileRepeat = 32.0f;

// This is the same two-height-channel wave equation used by Pezza. Red stores the
// current height and green stores the preceding height.
constexpr const char *WaterSimulationFragmentShader = R"GLSL(
uniform sampler2D texture;
uniform vec2 texel_size;
uniform float wave_speed;

const float wake_damping = 0.94;
const float settled_threshold = 0.0025;

vec2 fullValue(vec2 uv) { return texture2D(texture, uv).xy - 0.5; }
float heightAt(vec2 uv) { return fullValue(uv).x; }

void main() {
    vec2 uv = gl_TexCoord[0].xy;
    vec2 center = fullValue(uv);
    float laplacian =
        heightAt(uv + vec2( texel_size.x, 0.0)) +
        heightAt(uv + vec2(-texel_size.x, 0.0)) +
        heightAt(uv + vec2(0.0,  texel_size.y)) +
        heightAt(uv + vec2(0.0, -texel_size.y)) - center.x * 4.0;
    float height = clamp(center.x * 2.0 - center.y + laplacian * wave_speed,
                         -0.5, 0.5) * wake_damping;
    float previous = center.x;

    // A two-frame wave solver otherwise retains tiny oscillations indefinitely.
    // Once both height samples are visually flat, settle the texel exactly at rest.
    if (abs(height) < settled_threshold && abs(previous) < settled_threshold) {
        height = 0.0;
        previous = 0.0;
    }

    gl_FragColor = vec4(height + 0.5, previous + 0.5, 0.5, 1.0);
}
)GLSL";

// Port of Pezza's normal/refraction water renderer. Texture coordinates come
// from the world-sized water rectangle, so camera movement cannot detach wakes.
constexpr const char *WaterRenderFragmentShader = R"GLSL(
uniform sampler2D texture;
uniform sampler2D background;
uniform vec2 texel_size;

vec3 waterNormal(vec2 uv) {
    float hL = texture2D(texture, uv - vec2(texel_size.x, 0.0)).r;
    float hR = texture2D(texture, uv + vec2(texel_size.x, 0.0)).r;
    float hD = texture2D(texture, uv - vec2(0.0, texel_size.y)).r;
    float hU = texture2D(texture, uv + vec2(0.0, texel_size.y)).r;
    return normalize(vec3(-(hR - hL) * 0.4, -(hU - hD) * 0.4, 1.0));
}

void main() {
    vec2 uv = gl_TexCoord[0].xy;
    vec3 normal = waterNormal(uv);
    vec3 refracted = refract(vec3(0.0, 0.0, -1.0), normal, 1.0 / 1.33);
    vec2 backgroundUv = uv * 32.0 + refracted.xy * 8.0;
    vec3 tile = texture2D(background, backgroundUv).rgb;
    vec3 sea = vec3(0.129, 0.588, 0.953);
    float specular = pow(max(dot(normalize(vec3(5.0, 5.0, 100.0)), normal), 0.0), 1024.0);
    gl_FragColor = vec4(mix(sea, tile, 0.4) + vec3(specular), 1.0);
}
)GLSL";

sf::Vector2f Backend(const pipeframe::Vector2f value) { return {value.x,value.y}; }
pipeframe::Vector2f Normalized(const pipeframe::Vector2f value) {
    const float length = std::sqrt(value.x * value.x + value.y * value.y);
    return length > 0.0001f ? value / length : pipeframe::Vector2f{1.0f, 0.0f};
}

pipeframe::Vector2f Perpendicular(const pipeframe::Vector2f value) {
    return {-value.y, value.x};
}

} // namespace

bool SailBoatRenderer::LoadAssets(const std::filesystem::path &assetRoot,
                                  std::string &errorMessage) {
    const std::filesystem::path textureRoot = assetRoot / "Textures";
    resources.Clear();
    boatTexture=resources.LoadTexture((textureRoot/"seascape_1.png").string(),true,&errorMessage);
    boatDepthTexture=resources.LoadTexture((textureRoot/"seascape_1_depth.png").string(),true,&errorMessage);
    waterTexture=resources.LoadTexture((textureRoot/"water_tile_pipeframe.png").string(),true,&errorMessage);
    dottedTexture=resources.LoadTexture((textureRoot/"dotted.png").string(),true,&errorMessage);
    checkTexture=resources.LoadTexture((textureRoot/"check.png").string(),true,&errorMessage);
    font=resources.LoadFont((assetRoot/"Fonts"/"roboto_regular.ttf").string(),&errorMessage);
    if(resources.State(boatTexture)!=pipeframe::ResourceState::Ready||resources.State(boatDepthTexture)!=pipeframe::ResourceState::Ready||resources.State(waterTexture)!=pipeframe::ResourceState::Ready||resources.State(dottedTexture)!=pipeframe::ResourceState::Ready||resources.State(checkTexture)!=pipeframe::ResourceState::Ready||resources.State(font)!=pipeframe::ResourceState::Ready){UnloadAssets();return false;}
    auto *water=pipeframe::backend::sfml::GraphicsResourceAccess::Texture(resources,waterTexture);
    auto *dotted=pipeframe::backend::sfml::GraphicsResourceAccess::Texture(resources,dottedTexture);
    water->setRepeated(true);
    // Pezza mipmaps its final water surface. This keeps the repeated background tile
    // subtle when a complete course is fitted into the editor viewport.
    static_cast<void>(water->generateMipmap());
    dotted->setRepeated(true);
    waterSimulationShader=resources.LoadFragmentShader(WaterSimulationFragmentShader,&errorMessage);
    waterRenderShader=resources.LoadFragmentShader(WaterRenderFragmentShader,&errorMessage);
    waterShaderLoaded=resources.State(waterSimulationShader)==pipeframe::ResourceState::Ready&&resources.State(waterRenderShader)==pipeframe::ResourceState::Ready;
    assetsLoaded = true;
    errorMessage.clear();
    return true;
}

void SailBoatRenderer::UnloadAssets() {
    boatVertices.clear();
    wakeVertices.clear();
    trajectoryVertices.clear();
    statistics = {};
    waterTime = 0.0f;
    waterReadTexture = 0;
    resources.Clear();
    waterHeightTextures={};
    waterWorldSize = {};
    waterSimulationSize = {};
    waterSimulationInitialized = false;
    ResetRaceProgress();
    assetsLoaded = false;
    waterShaderLoaded = false;
}

void SailBoatRenderer::Advance(const float deltaTime) {
    if (std::isfinite(deltaTime) && deltaTime > 0.0f) {
        waterTime += deltaTime;
        reachedMarkPulse = std::max(0.0f, reachedMarkPulse - deltaTime);
    }
}

void SailBoatRenderer::SetRaceProgress(const std::size_t newReachedTargetCount) {
    if (newReachedTargetCount > reachedTargetCount) {
        reachedMarkPulse = 0.5f;
    } else if (newReachedTargetCount < reachedTargetCount) {
        reachedMarkPulse = 0.0f;
    }
    reachedTargetCount = newReachedTargetCount;
}

void SailBoatRenderer::ResetRaceProgress() {
    reachedMarkPulse = 0.0f;
    reachedTargetCount = 0;
}

void SailBoatRenderer::ResetWaterSimulation() {
    waterReadTexture = 0;
    for(const auto handle:waterHeightTextures) {
        if(auto *texture=pipeframe::backend::sfml::GraphicsResourceAccess::Surface(resources,handle)) {
            texture->clear(sf::Color{127, 127, 127, 255});
            texture->display();
        }
    }
    wakeVertices.clear();
    statistics.wakeSources = 0;
}

void SailBoatRenderer::DrawWater(sf::RenderTarget &target, const sf::Vector2f worldSize,
                                 const sf::Vector2f wind, const bool animateWater,
                                 const std::span<const SailBoatAgent> agents,
                                 const std::size_t bestAgentIndex,
                                 const bool onlyBestCreatesWake,
                                 const bool advanceSimulation) {
    static_cast<void>(wind);
    statistics.waterShaderActive = assetsLoaded && animateWater && waterShaderLoaded &&
                                   InitializeWaterSimulation(worldSize);
    statistics.wakeSources = 0;
    if (statistics.waterShaderActive) {
        DrawAnimatedWater(target, worldSize, agents, bestAgentIndex,
                          onlyBestCreatesWake, advanceSimulation);
    } else {
        DrawWaterPlaceholder(target, worldSize);
    }
}

bool SailBoatRenderer::InitializeWaterSimulation(const sf::Vector2f worldSize) {
    if (worldSize.x <= 0.0f || worldSize.y <= 0.0f) {
        return false;
    }
    if (waterSimulationInitialized && waterWorldSize == worldSize) {
        return true;
    }

    const float maximumWorldDimension = std::max(worldSize.x, worldSize.y);
    const float textureScale = std::min(
        PezzaWaterPixelsPerWorldUnit,
        static_cast<float>(MaximumWaterTextureDimension) / maximumWorldDimension);
    const sf::Vector2u size{
        std::max(1u, static_cast<unsigned int>(std::round(worldSize.x * textureScale))),
        std::max(1u, static_cast<unsigned int>(std::round(worldSize.y * textureScale)))};
    for(auto &handle:waterHeightTextures) {
        std::string error;
        handle=resources.CreateSurface(size.x,size.y,&error);
        auto *texture=pipeframe::backend::sfml::GraphicsResourceAccess::Surface(resources,handle);
        if(texture==nullptr) {
            waterSimulationInitialized = false;
            return false;
        }
        texture->setSmooth(true);
        texture->setRepeated(false);
        texture->clear(sf::Color{127, 127, 127, 255});
        texture->display();
    }
    waterReadTexture = 0;
    waterWorldSize = worldSize;
    waterSimulationSize = size;
    waterSimulationInitialized = true;
    return true;
}

void SailBoatRenderer::DrawAnimatedWater(sf::RenderTarget &target,
                                         const sf::Vector2f worldSize,
                                         const std::span<const SailBoatAgent> agents,
                                         const std::size_t bestAgentIndex,
                                         const bool onlyBestCreatesWake,
                                         const bool advanceSimulation) {
    statistics.wakeSources = 0;
    wakeVertices.clear();

    auto canCreateWake = [](const SailBoatAgent &agent) {
        const Boat &boat = agent.GetBoat();
        return !boat.HasCrashed() && !boat.HasFinished() && boat.GetSpeed() > 0.01f;
    };

    if (advanceSimulation && !agents.empty()) {
        if (onlyBestCreatesWake) {
            const std::size_t safeBestIndex = std::min(bestAgentIndex, agents.size() - 1);
            if (canCreateWake(agents[safeBestIndex])) {
                statistics.wakeSources = 1;
                wakeVertices.resize(6);
                WriteBoatQuad(wakeVertices, 0, agents[safeBestIndex].GetBoat(), sf::Color::White);
            }
        } else {
            for (const SailBoatAgent &agent : agents) {
                if (canCreateWake(agent)) {
                    ++statistics.wakeSources;
                }
            }
            wakeVertices.resize(statistics.wakeSources * 6);
            std::size_t wakeIndex = 0;
            for (const SailBoatAgent &agent : agents) {
                if (canCreateWake(agent)) {
                    const std::uint8_t alpha = static_cast<std::uint8_t>(
                        std::clamp(agent.GetBoat().GetSpeed(), 0.0f, 1.0f) * 255.0f);
                    WriteBoatQuad(wakeVertices, wakeIndex++, agent.GetBoat(),
                                  sf::Color{255, 255, 255, alpha});
                }
            }
        }
    }

    if (advanceSimulation) {
        auto *readSurface=pipeframe::backend::sfml::GraphicsResourceAccess::Surface(resources,waterHeightTextures[waterReadTexture]);
        auto *depthTexture=pipeframe::backend::sfml::GraphicsResourceAccess::Texture(resources,boatDepthTexture);
        if(!wakeVertices.empty()) {
            sf::RenderStates wakeStates;
            wakeStates.texture=depthTexture;
            wakeStates.transform.scale({static_cast<float>(waterSimulationSize.x) / worldSize.x,
                                        static_cast<float>(waterSimulationSize.y) / worldSize.y});
            readSurface->draw(wakeVertices.data(),wakeVertices.size(),sf::PrimitiveType::Triangles,wakeStates);
            readSurface->display();
        }

        const std::size_t writeTexture = 1 - waterReadTexture;
        sf::RectangleShape simulationStep(sf::Vector2f{waterSimulationSize});
        simulationStep.setTexture(&readSurface->getTexture());
        simulationStep.setTextureRect({{0, 0},
                                       {static_cast<int>(waterSimulationSize.x),
                                        static_cast<int>(waterSimulationSize.y)}});
        auto *simulationShader=pipeframe::backend::sfml::GraphicsResourceAccess::Shader(resources,waterSimulationShader);
        simulationShader->setUniform("texture",sf::Shader::CurrentTexture);
        simulationShader->setUniform(
            "texel_size", sf::Glsl::Vec2{1.0f / static_cast<float>(waterSimulationSize.x),
                                         1.0f / static_cast<float>(waterSimulationSize.y)});
        // The finite-difference coefficient scales with pixels-per-world-unit squared.
        // Without this correction our formerly low-resolution buffer propagated a
        // Pezza wave over roughly six times as much world space per frame.
        const float actualPixelsPerWorldUnit =
            static_cast<float>(waterSimulationSize.x) / worldSize.x;
        const float scaleRatio = actualPixelsPerWorldUnit / PezzaWaterPixelsPerWorldUnit;
        simulationShader->setUniform("wave_speed",0.30f*scaleRatio*scaleRatio);
        auto *writeSurface=pipeframe::backend::sfml::GraphicsResourceAccess::Surface(resources,waterHeightTextures[writeTexture]);
        writeSurface->draw(simulationStep,simulationShader);
        writeSurface->display();
        waterReadTexture = writeTexture;
    }

    // Pezza generates a mip chain for the rendered water every update. Our height
    // texture is also the rendered surface, so mipmap it before drawing it minified.
    auto *readSurface=pipeframe::backend::sfml::GraphicsResourceAccess::Surface(resources,waterHeightTextures[waterReadTexture]);
    static_cast<void>(readSurface->generateMipmap());

    sf::RectangleShape water(worldSize);
    water.setTexture(&readSurface->getTexture());
    water.setTextureRect({{0, 0},
                          {static_cast<int>(waterSimulationSize.x),
                           static_cast<int>(waterSimulationSize.y)}});
    auto *renderShader=pipeframe::backend::sfml::GraphicsResourceAccess::Shader(resources,waterRenderShader);
    renderShader->setUniform("texture",sf::Shader::CurrentTexture);
    renderShader->setUniform("background",*pipeframe::backend::sfml::GraphicsResourceAccess::Texture(resources,waterTexture));
    renderShader->setUniform(
        "texel_size", sf::Glsl::Vec2{1.0f / static_cast<float>(waterSimulationSize.x),
                                     1.0f / static_cast<float>(waterSimulationSize.y)});
    target.draw(water,renderShader);
}

void SailBoatRenderer::DrawWaterPlaceholder(sf::RenderTarget &target,
                                            const sf::Vector2f worldSize) {
    sf::RectangleShape water(worldSize);
    if (assetsLoaded) {
        const auto *texture=pipeframe::backend::sfml::GraphicsResourceAccess::Texture(resources,waterTexture);
        water.setTexture(texture);
        const int repeatExtent = static_cast<int>(
            static_cast<float>(texture->getSize().x)*SeaTileRepeat);
        water.setTextureRect({{0, 0}, {repeatExtent, repeatExtent}});
        target.draw(water);
    } else {
        water.setFillColor(sf::Color{33, 150, 243});
        target.draw(water);
    }

    sf::RectangleShape tint(worldSize);
    tint.setFillColor(SeaColor);
    target.draw(tint);
}

void SailBoatRenderer::DrawPopulation(sf::RenderTarget &target,
                                      const std::span<const SailBoatAgent> agents,
                                      const std::size_t bestAgentIndex,
                                      const bool drawGhosts,
                                      const bool highlightBest) {
    statistics.candidateBoats = agents.size();
    statistics.visibleBoats = 0;
    statistics.culledBoats = 0;
    statistics.ghostBoats = 0;
    statistics.populationDrawCalls = 0;
    boatVertices.resize(agents.size() * 6);

    const sf::View &view = target.getView();
    const sf::Vector2f viewHalfSize = view.getSize() * 0.5f;
    const sf::Vector2f viewMinimum = view.getCenter() - viewHalfSize - sf::Vector2f{BoatWorldLength, BoatWorldLength};
    const sf::Vector2f viewMaximum = view.getCenter() + viewHalfSize + sf::Vector2f{BoatWorldLength, BoatWorldLength};
    const auto isVisible = [&](const Boat &boat) {
        const sf::Vector2f position = Backend(boat.GetPosition());
        return position.x >= viewMinimum.x && position.y >= viewMinimum.y &&
               position.x <= viewMaximum.x && position.y <= viewMaximum.y;
    };

    std::size_t outputIndex = 0;
    for (std::size_t index = 0; index < agents.size(); ++index) {
        if (index == bestAgentIndex) {
            continue;
        }
        const Boat &boat = agents[index].GetBoat();
        if (!isVisible(boat)) {
            continue;
        }
        const std::uint8_t alpha = drawGhosts ? 50 : 255;
        const sf::Color color = boat.HasCrashed()
                                    ? sf::Color{CrashedColor.r, CrashedColor.g, CrashedColor.b, alpha}
                                    : sf::Color{255, 255, 255, alpha};
        WriteBoatQuad(boatVertices, outputIndex++, boat, color);
    }

    bool bestVisible = false;
    if (!agents.empty()) {
        const std::size_t safeBestIndex = std::min(bestAgentIndex, agents.size() - 1);
        const Boat &bestBoat = agents[safeBestIndex].GetBoat();
        if (isVisible(bestBoat)) {
            const sf::Color color = bestBoat.HasCrashed()
                                        ? CrashedColor
                                        : highlightBest ? BestColor : sf::Color::White;
            WriteBoatQuad(boatVertices, outputIndex++, bestBoat, color);
            bestVisible = true;
        }
    }

    boatVertices.resize(outputIndex * 6);
    statistics.visibleBoats = outputIndex;
    statistics.culledBoats = agents.size() - outputIndex;
    statistics.ghostBoats = outputIndex - (bestVisible ? 1u : 0u);
    statistics.boatVertices=boatVertices.size();
    sf::RenderStates states;
    if (assetsLoaded) {
        states.texture=pipeframe::backend::sfml::GraphicsResourceAccess::Texture(resources,boatTexture);
    }
    if(!boatVertices.empty()) {
        target.draw(boatVertices.data(),boatVertices.size(),sf::PrimitiveType::Triangles,states);
        statistics.populationDrawCalls = 1;
    }
}

void SailBoatRenderer::DrawBoat(sf::RenderTarget &target, const Boat &boat,
                                const sf::Color color) const {
    std::vector<sf::Vertex> vertices(6);
    WriteBoatQuad(vertices, 0, boat, color);
    sf::RenderStates states;
    if (assetsLoaded) {
        states.texture=pipeframe::backend::sfml::GraphicsResourceAccess::Texture(resources,boatTexture);
    }
    target.draw(vertices.data(),vertices.size(),sf::PrimitiveType::Triangles,states);
}

void SailBoatRenderer::DrawTrajectory(sf::RenderTarget &target, const Boat &boat) {
    const std::vector<TrajectoryPoint> &points = boat.GetTrajectory();
    trajectoryVertices.resize(points.size() * 2);
    for (std::size_t index = 0; index < points.size(); ++index) {
        const pipeframe::Vector2f normal = Perpendicular(Normalized(points[index].direction));
        trajectoryVertices[index * 2].position = Backend(points[index].position + normal * (RouteWidth * 0.5f));
        trajectoryVertices[index * 2 + 1].position = Backend(points[index].position - normal * (RouteWidth * 0.5f));
        trajectoryVertices[index * 2].color = BestColor;
        trajectoryVertices[index * 2 + 1].color = BestColor;
    }
    statistics.trajectoryVertices=trajectoryVertices.size();
    if(!trajectoryVertices.empty())target.draw(trajectoryVertices.data(),trajectoryVertices.size(),sf::PrimitiveType::TriangleStrip);
}

void SailBoatRenderer::DrawCourse(sf::RenderTarget &target, const RaceCourse &course,
                                  const SailBoatRaceTask &bestTask,
                                  const bool drawLabels) const {
    const std::size_t currentTarget = bestTask.GetTargetIndex();
    const float pulseScale = reachedMarkPulse > 0.0f
                                 ? 1.0f + 0.7f * (reachedMarkPulse / 0.5f)
                                 : 1.0f;

    const RaceSegment &start = course.GetStart();
    if (start.IsValid()) {
        DrawMark(target, Backend(start.GetFirstPoint()), 0, sf::Color::White, 1.0f);
        const pipeframe::Vector2f direction = Normalized(start.GetDirection());
        const pipeframe::Vector2f normal = Perpendicular(direction);
        const pipeframe::Vector2f origin = start.GetFirstPoint();
        std::vector<sf::Vertex> arrow(9);
        const std::array<pipeframe::Vector2f, 9> points = {
            origin + direction * 7.5f,
            origin + direction * 7.5f - normal * 1.5f,
            origin + direction * 7.5f - normal * 3.0f,
            origin + direction * 10.0f,
            origin + direction * 7.5f + normal * 3.0f,
            origin + direction * 7.5f + normal * 1.5f,
            origin + normal * 1.5f,
            origin - normal * 1.5f,
            origin + direction * 7.5f - normal * 1.5f,
        };
        for (std::size_t index = 0; index < points.size(); ++index) {
            arrow[index] = {Backend(points[index]), BestColor};
        }
        target.draw(arrow.data(),arrow.size(),sf::PrimitiveType::Triangles);
    }

    for (std::size_t index = 0; index < course.GetWaypoints().size(); ++index) {
        const bool reached = index < currentTarget;
        const bool mostRecent = reached && index + 1 == currentTarget;
        DrawMark(target, Backend(course.GetWaypoints()[index].segment.GetFirstPoint()),
                 drawLabels ? index + 1 : 0,
                 reached ? BestColor : CrashedColor,
                 mostRecent ? pulseScale : 1.0f);
    }

    if (!course.GetFinish().IsValid()) {
        return;
    }
    const std::size_t finishIndex = course.GetWaypoints().size();
    const bool finishReached = bestTask.HasFinished();
    DrawMark(target, Backend(course.GetFinish().GetFirstPoint()),
             drawLabels ? finishIndex + 1 : 0,
             finishReached ? BestColor : CrashedColor,
             finishReached ? pulseScale : 1.0f);

    if (finishReached && assetsLoaded) {
        const float checkSize = MarkRadius * 1.5f * pulseScale;
        sf::RectangleShape check({checkSize, checkSize});
        check.setOrigin({checkSize * 0.5f, checkSize * 0.5f});
        check.setPosition(Backend(course.GetFinish().GetFirstPoint()));
        check.setTexture(pipeframe::backend::sfml::GraphicsResourceAccess::Texture(resources,checkTexture));
        target.draw(check);
    }
}

void SailBoatRenderer::DrawNextTarget(sf::RenderTarget &target, const Boat &boat,
                                      const SailBoatRaceTask &task,
                                      const RaceCourse &course) const {
    if (task.HasFinished() || boat.HasCrashed() || course.GetTargetCount() == 0) {
        return;
    }
    const pipeframe::Vector2f targetPoint = task.GetTargetPoint(boat, course);
    const pipeframe::Vector2f difference = targetPoint - boat.GetPosition();
    const float distance = std::sqrt(difference.x * difference.x + difference.y * difference.y);
    if (distance < 0.001f) {
        return;
    }
    const pipeframe::Vector2f direction = difference / distance;
    const pipeframe::Vector2f normal = Perpendicular(direction);

    constexpr float DashLength = 5.0f;
    constexpr float GapLength = 3.0f;
    std::vector<sf::Vertex> dashes;
    for (float start = 12.0f; start < distance - 8.0f; start += DashLength + GapLength) {
        const float end = std::min(start + DashLength, distance - 8.0f);
        const pipeframe::Vector2f a = boat.GetPosition() + direction * start;
        const pipeframe::Vector2f b = boat.GetPosition() + direction * end;
        constexpr float HalfWidth = 0.35f;
        dashes.push_back({Backend(a-normal*HalfWidth),sf::Color{255,255,255,150}});
        dashes.push_back({Backend(b-normal*HalfWidth),sf::Color{255,255,255,150}});
        dashes.push_back({Backend(b+normal*HalfWidth),sf::Color{255,255,255,150}});
        dashes.push_back({Backend(a-normal*HalfWidth),sf::Color{255,255,255,150}});
        dashes.push_back({Backend(b+normal*HalfWidth),sf::Color{255,255,255,150}});
        dashes.push_back({Backend(a+normal*HalfWidth),sf::Color{255,255,255,150}});
    }
    if(!dashes.empty())target.draw(dashes.data(),dashes.size(),sf::PrimitiveType::Triangles);

    const pipeframe::Vector2f arrowCenter = boat.GetPosition() + direction * 16.0f;
    std::vector<sf::Vertex> arrow(3);
    arrow[0] = {Backend(arrowCenter + direction * 4.0f), sf::Color::White};
    arrow[1] = {Backend(arrowCenter - direction * 2.0f - normal * 2.0f), sf::Color::White};
    arrow[2] = {Backend(arrowCenter - direction * 2.0f + normal * 2.0f), sf::Color::White};
    target.draw(arrow.data(),arrow.size(),sf::PrimitiveType::Triangles);

    if (assetsLoaded) {
        sf::Text label(*pipeframe::backend::sfml::GraphicsResourceAccess::Font(resources,font),std::to_string(task.GetTargetIndex()+1),26);
        label.setScale({0.25f, 0.25f});
        label.setFillColor(sf::Color{20, 30, 40, 220});
        const sf::FloatRect bounds = label.getLocalBounds();
        label.setOrigin(bounds.position + bounds.size * 0.5f);
        label.setPosition(Backend(boat.GetPosition() + direction * 11.0f));
        target.draw(label);
    }
}

bool SailBoatRenderer::AreAssetsLoaded() const { return assetsLoaded; }
float SailBoatRenderer::GetWaterTime() const { return waterTime; }
const SailBoatRenderStatistics &SailBoatRenderer::GetStatistics() const { return statistics; }

void SailBoatRenderer::WriteBoatQuad(std::vector<sf::Vertex> &vertices,const std::size_t quadIndex,
                                     const Boat &boat, const sf::Color color) {
    const pipeframe::Vector2f forward = Normalized(boat.GetDirection());
    const pipeframe::Vector2f side = Perpendicular(forward);
    const pipeframe::Vector2f center = boat.GetPosition();
    const pipeframe::Vector2f halfForward = forward * (BoatWorldLength * 0.5f);
    const pipeframe::Vector2f halfSide = side * (BoatWorldWidth * 0.5f);

    const std::array<pipeframe::Vector2f, 4> positions = {
        center - halfForward - halfSide,
        center + halfForward - halfSide,
        center + halfForward + halfSide,
        center - halfForward + halfSide,
    };
    const std::array<sf::Vector2f, 4> textureCoordinates = {
        sf::Vector2f{0.0f, 0.0f},
        sf::Vector2f{BoatTextureSize.x, 0.0f},
        BoatTextureSize,
        sf::Vector2f{0.0f, BoatTextureSize.y},
    };
    constexpr std::array<std::size_t, 6> Indices{0, 1, 2, 0, 2, 3};
    for (std::size_t index = 0; index < Indices.size(); ++index) {
        sf::Vertex &vertex = vertices[quadIndex * 6 + index];
        vertex.position = Backend(positions[Indices[index]]);
        vertex.texCoords = textureCoordinates[Indices[index]];
        vertex.color = color;
    }
}

void SailBoatRenderer::DrawMark(sf::RenderTarget &target, const sf::Vector2f position,
                                const std::size_t labelValue, const sf::Color outlineColor,
                                const float scale) const {
    sf::CircleShape mark(MarkRadius, 48);
    mark.setOrigin({MarkRadius, MarkRadius});
    mark.setPosition(position);
    mark.setScale({scale, scale});
    mark.setFillColor(sf::Color::White);
    mark.setOutlineColor(outlineColor);
    mark.setOutlineThickness(0.75f);
    target.draw(mark);

    if (!assetsLoaded || labelValue == 0) {
        return;
    }
    sf::Text label(*pipeframe::backend::sfml::GraphicsResourceAccess::Font(resources,font),std::to_string(labelValue),28);
    label.setScale({0.2f * scale, 0.2f * scale});
    label.setFillColor(sf::Color{0, 0, 0, 165});
    const sf::FloatRect bounds = label.getLocalBounds();
    label.setOrigin(bounds.position + bounds.size * 0.5f);
    label.setPosition(position);
    target.draw(label);
}

} // namespace sailboat_simulation
