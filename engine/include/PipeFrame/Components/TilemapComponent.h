#pragma once
#include <PipeFrame/Project/ComponentSchema.h>
namespace pipeframe {
inline constexpr const char *TilemapComponentTypeId="pipeframe.tilemap2d";
struct TilemapComponent {
    AssetReference asset;
    bool visible{true};
    static auto Schema(){return ComponentSchema<TilemapComponent>(TilemapComponentTypeId,"Tilemap")
        .Editable({.key="asset",.displayName="Tilemap Asset",.kind=PropertyKind::AssetReference,.defaultValue=AssetReference{},.editorHint="asset:Tilemap"},&TilemapComponent::asset)
        .Editable({.key="visible",.displayName="Visible",.kind=PropertyKind::Boolean,.defaultValue=true},&TilemapComponent::visible);}
};
}
