

#ifndef PIPEFRAME_TILEMAPRENDERER_H
#define PIPEFRAME_TILEMAPRENDERER_H


#include "TileMap.h"
#include <SDL3/SDL.h>

class TilemapRenderer
{
public:
    void Render(SDL_Renderer* renderer, SDL_Texture* tilesetTexture, const TileMap& tilemap,
                const SDL_FRect& camera) const;
};


#endif //PIPEFRAME_TILEMAPRENDERER_H
