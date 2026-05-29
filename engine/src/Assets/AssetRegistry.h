#ifndef PIPEFRAME_ASSETREGISTRY_H
#define PIPEFRAME_ASSETREGISTRY_H

#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

enum class TextureSpriteMode
{
    SingleImage,
    SpriteSheet
};

struct TextureSpriteMetadata
{
    TextureSpriteMode mode = TextureSpriteMode::SingleImage;
    int defaultDisplayWidth = 32;
    int defaultDisplayHeight = 32;
    int frameWidth = 0;
    int frameHeight = 0;
    int defaultFrame = 0;
};

struct TextureInfo
{
    std::string id;
    std::string filePath;
    float pixelWidth = 0.0f;
    float pixelHeight = 0.0f;
    TextureSpriteMetadata sprite;
};

class AssetRegistry
{
private:
    std::map<std::string, SDL_Texture*> textures;
    std::map<std::string, TextureInfo> textureInfos;
    std::map<std::string, TTF_Font*> fonts;
    std::unordered_set<std::string> missingTextureWarnings;
    // TODO: create a map for fonts
    // TODO: create a map for audio

public:
    AssetRegistry();
    ~AssetRegistry();

    void ClearAssets();

    void AddTexture(
        SDL_Renderer* renderer,
        const std::string& assetId,
        const std::string& filePath,
        const TextureSpriteMetadata& spriteMetadata = TextureSpriteMetadata()
    );
    SDL_Texture* GetTexture(const std::string& assetId);
    const TextureInfo* GetTextureInfo(const std::string& assetId) const;
    std::vector<std::string> GetTextureIds() const;

    void AddFont(const std::string& assetId, const std::string& filePath, int fontSize);
    TTF_Font* GetFont(const std::string& assetId);
};

#endif // PIPEFRAME_ASSETREGISTRY_H
