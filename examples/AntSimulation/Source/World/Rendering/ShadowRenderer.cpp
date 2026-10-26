#include "World/Rendering/ShadowRenderer.h"

namespace ant_simulation {

namespace {

constexpr const char *AlphaMaskFragmentShader = R"GLSL(
uniform sampler2D texture;

void main() {
    float alpha = texture2D(texture, gl_TexCoord[0].xy).a * gl_Color.a;
    gl_FragColor = vec4(gl_Color.rgb, alpha);
}
)GLSL";

} // namespace

bool ShadowRenderer::LoadAssets(const std::filesystem::path &assetRoot, std::string &errorMessage) {
    const std::filesystem::path textureRoot = assetRoot / "Textures";

    resources.Clear();
    bodyShadowTexture =
        resources.LoadTexture((textureRoot / "ant_body_parts_shadow.png").string(), true, &errorMessage);
    legShadowTexture = resources.LoadTexture((textureRoot / "ant_leg_shadow.png").string(), true, &errorMessage);
    circleTexture = resources.LoadTexture((textureRoot / "circle.png").string(), true, &errorMessage);
    fullAntTexture = resources.LoadTexture((textureRoot / "ant_full.png").string(), true, &errorMessage);
    assetsLoaded = resources.State(bodyShadowTexture) == pipeframe::ResourceState::Ready &&
                   resources.State(legShadowTexture) == pipeframe::ResourceState::Ready &&
                   resources.State(circleTexture) == pipeframe::ResourceState::Ready &&
                   resources.State(fullAntTexture) == pipeframe::ResourceState::Ready;
    if (!assetsLoaded)
        return false;
    alphaMaskShader = resources.LoadFragmentShader(AlphaMaskFragmentShader, &errorMessage);
    shaderLoaded = resources.State(alphaMaskShader) == pipeframe::ResourceState::Ready;

    assetsLoaded = true;
    errorMessage.clear();

    return true;
}

void ShadowRenderer::SetEnabled(const bool newEnabled) { enabled = newEnabled; }

void ShadowRenderer::SetOffset(const pipeframe::Vector2f newOffset) { offset = newOffset; }

void ShadowRenderer::SetColor(const pipeframe::Color newColor) { color = newColor; }

void ShadowRenderer::Update(const AntGeometry &sourceGeometry, const AntRenderingMode newMode) {
    mode = newMode;

    if (!enabled || mode == AntRenderingMode::Points) {
        bodyVertices.clear();
        legVertices.clear();
        foodVertices.clear();

        return;
    }

    CopyAndTransform(sourceGeometry.GetBodyVertices(), bodyVertices);

    CopyAndTransform(sourceGeometry.GetFoodVertices(), foodVertices);

    if (mode == AntRenderingMode::DetailedQuads) {
        CopyAndTransform(sourceGeometry.GetLegVertices(), legVertices);
    } else {
        legVertices.clear();
    }
}

void ShadowRenderer::Draw(pipeframe::Canvas target, pipeframe::RenderState states) const {
    if (!enabled || mode == AntRenderingMode::Points) {
        return;
    }

    states.blendMode = pipeframe::BlendMode::Alpha;
    states.shader = shaderLoaded ? pipeframe::ShaderBinding(resources, alphaMaskShader) : nullptr;

    if (mode == AntRenderingMode::DetailedQuads) {
        DrawVertices(target, legVertices,
                     assetsLoaded ? pipeframe::TextureBinding(resources, legShadowTexture) : nullptr, states);

        DrawVertices(target, foodVertices, assetsLoaded ? pipeframe::TextureBinding(resources, circleTexture) : nullptr,
                     states);

        DrawVertices(target, bodyVertices,
                     assetsLoaded ? pipeframe::TextureBinding(resources, bodyShadowTexture) : nullptr, states);

    } else {
        DrawVertices(target, foodVertices, assetsLoaded ? pipeframe::TextureBinding(resources, circleTexture) : nullptr,
                     states);

        DrawVertices(target, bodyVertices,
                     assetsLoaded ? pipeframe::TextureBinding(resources, fullAntTexture) : nullptr, states);
    }
}

bool ShadowRenderer::IsEnabled() const { return enabled; }

bool ShadowRenderer::AreAssetsLoaded() const { return assetsLoaded; }

std::span<const pipeframe::Vertex2D> ShadowRenderer::GetBodyVertices() const { return bodyVertices; }

std::span<const pipeframe::Vertex2D> ShadowRenderer::GetLegVertices() const { return legVertices; }

std::span<const pipeframe::Vertex2D> ShadowRenderer::GetFoodVertices() const { return foodVertices; }

void ShadowRenderer::CopyAndTransform(const std::span<const pipeframe::Vertex2D> source,
                                      std::vector<pipeframe::Vertex2D> &destination) const {
    destination.assign(source.begin(), source.end());

    for (pipeframe::Vertex2D &vertex : destination) {
        if (vertex.color.a == 0) {
            vertex.color = pipeframe::Color::Transparent;

            continue;
        }

        vertex.position += offset;
        vertex.color = color;
    }
}

void ShadowRenderer::DrawVertices(pipeframe::Canvas target, const std::span<const pipeframe::Vertex2D> vertices,
                                  pipeframe::TextureBinding texture, pipeframe::RenderState states) {
    if (vertices.empty()) {
        return;
    }

    states.texture = texture;

    target.Draw(vertices.data(), vertices.size(), pipeframe::PrimitiveTopology::Triangles, states);
}

} // namespace ant_simulation
