#ifndef PIPEFRAME_EDITORTILEPALETTEPANEL_H
#define PIPEFRAME_EDITORTILEPALETTEPANEL_H

#include <SDL3/SDL.h>

#include "EditorSessionState.h"
#include "Map/TileMap.h"

class EditorTilePalettePanel
{
public:
    void Draw(
        const TileMap* tileMap,
        SDL_Texture* tilePaletteTexture,
        EditorSessionState& state
    );

private:
    const char* TerrainTypeToString(TerrainType terrain) const;
};

#endif // PIPEFRAME_EDITORTILEPALETTEPANEL_H
