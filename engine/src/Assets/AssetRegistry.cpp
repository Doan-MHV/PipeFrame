#include "AssetRegistry.h"
#include "Logger/Logger.h"
#include <SDL3_image//SDL_image.h>

AssetRegistry::AssetRegistry()
{
    Logger::Log("AssetRegistry constructor called!");
}

AssetRegistry::~AssetRegistry()
{
    ClearAssets();
    Logger::Log("AssetRegistry destructor called!");
}

void AssetRegistry::ClearAssets()
{
    for (auto texture : textures)
    {
        SDL_DestroyTexture(texture.second);
    }
    textures.clear();
    textureInfos.clear();
    missingTextureWarnings.clear();
}

void AssetRegistry::AddTexture(
    SDL_Renderer* renderer,
    const std::string& assetId,
    const std::string& filePath,
    const TextureSpriteMetadata& spriteMetadata
)
{
    SDL_Surface* surface = IMG_Load(filePath.c_str());
    if (!surface)
    {
        Logger::Err("Failed to load texture " + assetId + " from " + filePath + ": " + SDL_GetError());
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    if (!texture)
    {
        Logger::Err("Failed to create texture " + assetId + " from " + filePath + ": " + SDL_GetError());
        return;
    }

    float textureWidth = 0.0f;
    float textureHeight = 0.0f;
    SDL_GetTextureSize(texture, &textureWidth, &textureHeight);

    // Add the texture to the map
    auto existingTexture = textures.find(assetId);
    if (existingTexture != textures.end())
    {
        SDL_DestroyTexture(existingTexture->second);
        existingTexture->second = texture;
    }
    else
    {
        textures.emplace(assetId, texture);
    }

    TextureInfo info;
    info.id = assetId;
    info.filePath = filePath;
    info.pixelWidth = textureWidth;
    info.pixelHeight = textureHeight;
    info.sprite = spriteMetadata;

    if (info.sprite.defaultDisplayWidth <= 0)
    {
        info.sprite.defaultDisplayWidth = static_cast<int>(textureWidth);
    }

    if (info.sprite.defaultDisplayHeight <= 0)
    {
        info.sprite.defaultDisplayHeight = static_cast<int>(textureHeight);
    }

    if (info.sprite.mode == TextureSpriteMode::SpriteSheet)
    {
        if (info.sprite.frameWidth <= 0)
        {
            info.sprite.frameWidth = info.sprite.defaultDisplayWidth;
        }

        if (info.sprite.frameHeight <= 0)
        {
            info.sprite.frameHeight = info.sprite.defaultDisplayHeight;
        }
    }

    textureInfos[assetId] = info;

    Logger::Log("Texture added to the AssetRegistry with id " + assetId);
}

SDL_Texture* AssetRegistry::GetTexture(const std::string& assetId)
{
    if (assetId.empty())
    {
        return nullptr;
    }

    const auto texture = textures.find(assetId);
    if (texture == textures.end())
    {
        if (!missingTextureWarnings.contains(assetId))
        {
            Logger::Err("Texture not found in AssetRegistry: " + assetId);
            missingTextureWarnings.insert(assetId);
        }

        return nullptr;
    }

    return texture->second;
}

const TextureInfo* AssetRegistry::GetTextureInfo(const std::string& assetId) const
{
    const auto textureInfo = textureInfos.find(assetId);
    if (textureInfo == textureInfos.end())
    {
        return nullptr;
    }

    return &textureInfo->second;
}

std::vector<std::string> AssetRegistry::GetTextureIds() const
{
    std::vector<std::string> ids;
    ids.reserve(textures.size());

    for (const auto& pair : textures)
    {
        ids.push_back(pair.first);
    }

    return ids;
}


void AssetRegistry::AddFont(const std::string& assetId, const std::string& filePath, int fontSize)
{
    fonts.emplace(assetId, TTF_OpenFont(filePath.c_str(), fontSize));
    Logger::Log("AssetRegistry added to the AssetRegistry with id " + assetId);
}

TTF_Font* AssetRegistry::GetFont(const std::string& assetId)
{
    return fonts[assetId];
}
