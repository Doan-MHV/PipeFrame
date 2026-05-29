#include "TileMapRenderer.h"

void TilemapRenderer::Render(SDL_Renderer* renderer, SDL_Texture* tilesetTexture, const TileMap& tilemap,
                             const SDL_FRect& camera) const
{
    const int tileSize = tilemap.GetTileSize();
    const float scale = tilemap.GetScale();
    const int scaledTileSize = static_cast<int>(tileSize * scale);

    for (int row = 0; row < tilemap.GetRows(); row++)
    {
        for (int col = 0; col < tilemap.GetCols(); col++)
        {
            const TileCell& tile = tilemap.GetTile(row, col);

            const SDL_FRect dstRect = {
                (col * scaledTileSize) - camera.x,
                (row * scaledTileSize) - camera.y,
                static_cast<float>(scaledTileSize),
                static_cast<float>(scaledTileSize)
            };

            // Skip tiles completely outside the camera view
            if (dstRect.x + dstRect.w < 0 || dstRect.x > camera.w ||
                dstRect.y + dstRect.h < 0 || dstRect.y > camera.h)
            {
                continue;
            }

            const SDL_FRect srcRect = {
                static_cast<float>(tile.tilesetColumn * tileSize),
                static_cast<float>(tile.tilesetRow * tileSize),
                static_cast<float>(tileSize),
                static_cast<float>(tileSize)
            };

            SDL_RenderTexture(renderer, tilesetTexture, &srcRect, &dstRect);
        }
    }
}
