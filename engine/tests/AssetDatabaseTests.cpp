#include <PipeFrame/Project/AssetDatabase.h>
#include <PipeFrame/Editor/TilemapAssetEditor.h>
#include <PipeFrame/Resources/ImageData.h>
#include <PipeFrame/Environment/TilemapSerializer.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>

namespace {
void Require(const bool value, const char *message) {
    if (!value) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
void Write(const std::filesystem::path &path, const std::string &contents) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << contents;
}
}

int main() {
    using namespace pipeframe;
    using namespace pipeframe::assets;
    const auto root = std::filesystem::temp_directory_path() / "pipeframe_asset_database_test";
    const auto sources = std::filesystem::temp_directory_path() / "pipeframe_asset_sources";
    std::filesystem::remove_all(root); std::filesystem::remove_all(sources);
    std::filesystem::create_directories(sources);

    const auto discoveryRoot = root / "discovery-project";
    std::string discoveryError;
    Require(SaveImageData(sources / "discovery.png", ImageData({8,8}, {80,120,160,255}), discoveryError), "Create discovery texture");
    std::filesystem::create_directories(discoveryRoot / "Assets/Textures/Nested");
    std::filesystem::copy_file(sources / "discovery.png", discoveryRoot / "Assets/Textures/Nested/ant.png");
    Write(discoveryRoot / "Assets/notes.txt", "unsupported");
    AssetDatabase discovered;
    Require(discovered.Open(discoveryRoot, &discoveryError) && discovered.GetAssets().size()==1,
            "Opening a project discovers nested textures without registration");
    const auto discoveredId=discovered.GetAssets().front().id;
    Require(discovered.GetAssets().front().sourcePath=="Assets/Textures/Nested/ant.png", "Discovery imports in place");
    Require(discovered.DiscoverProjectAssets() && discovered.GetAssets().size()==1 &&
            discovered.Find(discoveredId)->revision==1, "Repeated discovery preserves IDs and cache revision");
    Write(discoveryRoot / "Assets/new.glsl", "void main() {}");
    Require(discovered.DiscoverProjectAssets() && discovered.GetAssets().size()==2, "Discovery finds files added after open");
    AssetDatabase reopenedDiscovery;
    Require(reopenedDiscovery.Open(discoveryRoot) && reopenedDiscovery.GetAssets().size()==2 &&
            reopenedDiscovery.Find(discoveredId), "Discovered IDs persist across reopen");
    // A disposable cache can be partially or entirely removed without losing asset identity.
    const auto imageCache=discoveryRoot/reopenedDiscovery.Find(discoveredId)->cachePath.parent_path()/"image.png";
    std::filesystem::remove(imageCache);
    AssetDatabase repairedDiscovery;
    Require(repairedDiscovery.Open(discoveryRoot) && repairedDiscovery.Find(discoveredId)->revision==2 &&
            std::filesystem::is_regular_file(imageCache), "Reopen repairs a missing decoded texture without changing its ID");
    std::filesystem::remove_all(discoveryRoot/".pipeframe/cache");
    Require(repairedDiscovery.DiscoverProjectAssets() && repairedDiscovery.Find(discoveredId)->revision==3 &&
            std::filesystem::is_regular_file(imageCache), "Discovery rebuilds a deleted cache from original Assets files");
    const auto index=discoveryRoot/".pipeframe/assets.db";
    std::ifstream savedIndex(index);std::string oldIndex((std::istreambuf_iterator<char>(savedIndex)),{});savedIndex.close();
    const auto version=oldIndex.find("\"pipeframe.texture\" 4 ");
    Require(version!=std::string::npos,"Fixture contains the current texture importer version");
    oldIndex.replace(version,std::string("\"pipeframe.texture\" 4 ").size(),"\"pipeframe.texture\" 3 ");Write(index,oldIndex);
    AssetDatabase upgradedDiscovery;
    Require(upgradedDiscovery.Open(discoveryRoot) && upgradedDiscovery.Find(discoveredId)->importerVersion==4 &&
            upgradedDiscovery.Find(discoveredId)->revision==4,"Opening upgrades an old importer even when its cache files exist");
    Require(upgradedDiscovery.DiscoverProjectAssets() && upgradedDiscovery.Find(discoveredId)->revision==4,
            "A healthy upgraded cache is not rebuilt repeatedly");
    std::filesystem::remove_all(discoveryRoot);
    // Exercise the actual Ant texture formats in an isolated project, leaving its index untouched.
    const auto antAssets=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() /
        "examples/AntSimulation/Assets";
    std::filesystem::create_directories(discoveryRoot / "Assets");
    std::filesystem::copy(antAssets / "Textures", discoveryRoot / "Assets/Textures",
                          std::filesystem::copy_options::recursive);
    AssetDatabase antDiscovery;
    Require(antDiscovery.Open(discoveryRoot), "Open project containing Ant textures");
    std::size_t antTextureCount=0;
    for(const auto &entry:std::filesystem::directory_iterator(antAssets / "Textures"))
        if(entry.is_regular_file()) ++antTextureCount;
    Require(antDiscovery.GetAssets().size()==antTextureCount, "Every current Ant texture is automatically indexed");
    for(const auto &record:antDiscovery.GetAssets())
        Require(record.type==AssetType::Texture && record.state==AssetState::Ready &&
                std::filesystem::is_regular_file(discoveryRoot/record.thumbnailPath), "Ant textures decode with thumbnails");
    std::filesystem::remove_all(discoveryRoot);


    const std::vector<std::pair<std::string,AssetType>> fixtures{
        {"albedo.png",AssetType::Texture},{"tone.wav",AssetType::Audio},
        {"surface.glsl",AssetType::Shader},{"robot.pfmat",AssetType::Material},
        {"chassis.obj",AssetType::Model},{"arena.pfscene",AssetType::Scene},
        {"robot.pfprefab",AssetType::Prefab}};
    for (const auto &[name,type] : fixtures) {
        std::string contents="fixture-"+name;
        if (type == AssetType::Texture) {
            std::string error;
            Require(SaveImageData(sources/name, ImageData({1024, 512}, {80, 120, 160, 96}), error), "Write real texture fixture.");
            continue;
        }
        if(type==AssetType::Material)contents="PIPEFRAME_MATERIAL2D 1\n\"\"\n255 255 255 255\n1 1 0\n";
        if(type==AssetType::Audio)contents="RIFF0000WAVEfixture";
        if(type==AssetType::Model)contents="o chassis\nv 0 0 0\n";
        Write(sources/name,contents);
    }
    Write(sources/"wheel.pfpart","generic-part");

    AssetDatabase database; std::string error;
    Require(database.Open(root,&error),"A project asset database should open.");
    std::set<AssetType> importedTypes;
    AssetId textureId;
    for (const auto &[name,type] : fixtures) {
        const auto id=database.ImportNow({sources/name},&error);
        Require(id.has_value(),"Every built-in asset importer should accept its extension.");
        Require(database.Find(*id)->cachePath!=database.Find(*id)->previewPath&&
                database.Find(*id)->previewPath!=database.Find(*id)->thumbnailPath&&
                std::filesystem::is_regular_file(root/database.Find(*id)->previewPath)&&
                std::filesystem::is_regular_file(root/database.Find(*id)->thumbnailPath),
                "Built-in importers must produce distinct deterministic cache, preview, and thumbnail artifacts.");
        importedTypes.insert(database.Find(*id)->type);
        if(type==AssetType::Texture) textureId=*id;
    }
    Require(importedTypes.size()==fixtures.size(),"Texture, audio, shader, material, model, scene, and prefab importers should be distinct.");

    ImageData preview, thumbnail;
    Require(LoadImageData(root/database.Find(textureId)->previewPath, preview, error) &&
            LoadImageData(root/database.Find(textureId)->thumbnailPath, thumbnail, error),
            "Texture previews must decode as actual images.");
    Require(preview.Size() == Vector2u{512, 256} && thumbnail.Size() == Vector2u{128, 64} &&
            thumbnail.Pixel({0, 0}) == Color{80, 120, 160, 96},
            "Previews must preserve aspect ratio, colour and alpha within size bounds.");
    Tilemap2D environment(4,4);environment.DefineTile({1,{90,80,70,255},true});environment.AddLayer("Walls");
    environment.SetTile(0,{2,2},1);
    {std::ofstream output(sources/"environment.pftilemap");Require(TilemapSerializer::Save(environment,output),"Write tilemap asset.");}
    const auto tilemapId=database.ImportNow({sources/"environment.pftilemap"},&error);
    Require(tilemapId && database.Find(*tilemapId)->type==AssetType::Tilemap &&
            database.Search({.type=AssetType::Tilemap}).size()==1,"Tilemaps must import through the existing asset database.");
    TilemapAssetEditor editor;
    Require(editor.Open(database,*tilemapId) && !editor.IsDirty(), "Open a private tilemap editing document");
    editor.Configure(0,std::make_shared<TileBrush>(1)); editor.SetEnabled(true);
    const InputEvent down{InputEventType::PointerPressed,PointerInput{PointerButton::Left,{}}};
    const InputEvent up{InputEventType::PointerReleased,PointerInput{PointerButton::Left,{}}};
    auto cell=[](int x,int y){return std::optional<GridCoordinate>{{x,y}};};
    editor.HandleEvent(down,cell(1,1),true);
    Require(editor.HasPointerCapture() && !editor.Save(), "Saving cannot commit an unfinished stroke");
    editor.HandleEvent(up,cell(3,1));
    Require(editor.IsDirty() && editor.CanUndo() && !editor.Close() && !editor.Open(database,*tilemapId),
            "Dirty document cannot be silently replaced or closed");
    Require(editor.Undo() && !editor.IsDirty() && editor.CanRedo(), "Undo to initial state clears dirty flag");
    Require(editor.Redo() && editor.IsDirty(), "Redo restores stroke and dirty flag");
    const auto oldRevision=database.Find(*tilemapId)->revision;
    Require(editor.Save() && !editor.IsDirty() && database.Find(*tilemapId)->revision==oldRevision+1,
            "Save republishes the same asset ID with a new cache revision");
    std::ifstream editedSource(root/database.Find(*tilemapId)->sourcePath);
    auto editedMap=TilemapSerializer::Load(editedSource,error);
    Require(editedMap && editedMap->Tile(0,{3,1})==1, "Saved stroke survives source reload");
    Require(editor.Save() && database.Find(*tilemapId)->revision==oldRevision+1,"Clean save does not reimport");
    Require(editor.Undo() && editor.IsDirty(), "Undo across saved cursor marks dirty");
    editor.HandleEvent(down,cell(3,2),true); editor.HandleEvent(up,cell(3,2));
    Require(editor.IsDirty() && !editor.CanRedo(), "Branching invalidates redo and the old save point");
    Require(editor.Undo() && editor.IsDirty(), "Discarded save point cannot be mistaken for clean state");
    const auto sourcePath=root/database.Find(*tilemapId)->sourcePath;
    std::ifstream beforeExternal(sourcePath); std::ostringstream preserved; preserved<<beforeExternal.rdbuf();
    Write(sourcePath,preserved.str()+"\n");
    Require(!editor.Save() && editor.IsDirty(), "External source edits must not be overwritten");
    Require(editor.Close(true) && editor.Open(database,*tilemapId) && !editor.IsDirty(), "Explicit discard/reopen recovers external changes");
    editor.Configure(0,std::make_shared<TileBrush>(0)); editor.SetEnabled(true);
    editor.HandleEvent(down,cell(1,1),true);
    editor.HandleEvent({InputEventType::KeyPressed,KeyInput{InputKey::Escape}});
    Require(!editor.IsDirty() && !editor.CanUndo(), "Cancelled stroke produces no history or dirty state");
    editor.HandleEvent(down,cell(1,1),true); editor.HandleEvent(up,cell(1,1));
    Write(sourcePath.string()+".pipeframe-saving","existing temporary data");
    Require(!editor.Save() && editor.IsDirty(), "Existing temporary file blocks replacement safely");
    std::filesystem::remove(sourcePath.string()+".pipeframe-saving");
    Require(editor.Save(), "Saving can retry after temporary-file conflict");
    const auto copy=editor.CopyAsset();
    Require(copy && *copy!=*tilemapId && editor.AssetId()==*tilemapId && !editor.IsDirty(),
            "Copy creates a new stable ID without switching or modifying the original document");
    TilemapAssetEditor copiedEditor;
    Require(copiedEditor.Open(database,*copy) && copiedEditor.Document()->Tile(0,{3,1})==editor.Document()->Tile(0,{3,1}),
            "Copied map retains authored cells");
    copiedEditor.Configure(0,std::make_shared<TileBrush>(1));copiedEditor.SetEnabled(true);
    copiedEditor.HandleEvent(down,cell(0,0),true);copiedEditor.HandleEvent(up,cell(0,0));
    Require(!copiedEditor.CopyAsset() && copiedEditor.IsDirty(),"Copy refuses unsaved changes without discarding them");
    Require(copiedEditor.Save() && editor.Document()->Tile(0,{0,0})==0,"Painting copy leaves original unchanged");
    copiedEditor.Close();editor.Close();
    Require(editor.Open(database,*tilemapId)&&editor.Document()->Tile(0,{0,0})==0,
            "Original saved source remains unchanged after painting copy");
    Require(copiedEditor.Open(database,*copy)&&copiedEditor.Document()->Tile(0,{0,0})==1,
            "Copied saved source retains its independent paint after reopen");
    copiedEditor.Close();editor.Close();

    Write(sources/"broken.pftilemap","PIPEFRAME_TILEMAP 1\n4 4");
    Require(!database.ImportNow({sources/"broken.pftilemap"},&error),"Tilemap importer must validate its actual contents.");
    PartAssetMetadata part;
    part.category="drive"; part.physicalSize={0.065f,0.026f}; part.collisionShape="circle";
    part.attachments.push_back({"axle","mechanical",{}, {"mechanical"},false});
    part.compatibilityTags={"differential-drive","small-chassis"};
    part.configurationLimits.push_back({"angularSpeed",0,22,"rad/s"});
    const auto partId=database.ImportNow({sources/"wheel.pfpart",{},AssetType::Part,{textureId},{"wheel","actuator"},part},&error);
    Require(partId && database.Find(*partId)->part==part,"Generic part metadata should import without brand-specific fields.");
    Require(database.GetDependents(textureId)==std::vector<AssetId>{*partId},"Asset dependencies should be queryable.");
    Require(database.Search({.text="wheel",.type=AssetType::Part,.tag="actuator"}).size()==1,
            "The browser query should combine text, type, and tags.");

    const auto originalRevision=database.Find(textureId)->revision;
    const auto originalHash=database.Find(textureId)->contentHash;
    Require(database.Move(textureId,"Textures/Moved/albedo.png",&error),"Moving an asset should succeed.");
    Require(database.Find(textureId)->sourcePath==std::filesystem::path("Assets/Textures/Moved/albedo.png"),
            "Moving should preserve the stable ID and update only the source path.");
    Require(SaveImageData(root/database.Find(textureId)->sourcePath, ImageData({32, 16}, {10, 20, 30, 255}), error), "Write changed texture.");
    Require(database.Reimport(textureId,&error),"A changed asset should reimport.");
    Require(database.Find(textureId)->revision==originalRevision+1 && database.Find(textureId)->contentHash!=originalHash,
            "Reimport should preserve identity while advancing revision and content hash.");

    Require(LoadImageData(root/database.Find(textureId)->thumbnailPath, thumbnail, error) &&
            thumbnail.Size() == Vector2u{32, 16} && thumbnail.Pixel({0, 0}) == Color{10, 20, 30, 255},
            "Reimport must replace the preview without upscaling small textures.");
    Write(sources/"truncated.png", std::string{"\x89PNG\r\n\x1a\n", 8}+"fake-payload");
    Require(!database.ImportNow({sources/"truncated.png"}, &error), "Reject a valid signature with corrupt payload.");
    const auto validRevision = database.Find(textureId)->revision;
    Write(root/database.Find(textureId)->sourcePath, std::string{"\x89PNG\r\n\x1a\n", 8}+"corrupt-reimport");
    Require(!database.Reimport(textureId, &error) && database.Find(textureId)->revision == validRevision &&
            LoadImageData(root/database.Find(textureId)->thumbnailPath, thumbnail, error) &&
            thumbnail.Pixel({0, 0}) == Color{10, 20, 30, 255},
            "A decode failure must not overwrite the last valid thumbnail or advance revision.");
    const auto missingPath=root/database.Find(textureId)->sourcePath;
    std::filesystem::remove(missingPath); database.RefreshMissingStates();
    Require(database.Find(textureId)->state==AssetState::Missing,"A missing source should be visible in the database.");
    Require(SaveImageData(sources/"replacement.png", ImageData({8, 8}, {40, 50, 60, 255}), error), "Write replacement texture.");
    Require(database.RepairMissingSource(textureId,sources/"replacement.png",&error) &&
            database.Find(textureId)->state==AssetState::Ready,
            "Missing source repair should keep the reference ID and reimport the replacement.");

    const auto cancelled=database.QueueImport({sources/"tone.wav"});
    Require(database.CancelOperation(cancelled) && !database.ProcessOperation(cancelled) &&
            database.GetOperations().back().state==AssetOperationState::Cancelled,
            "Queued imports should be visible and cancellable.");
    Write(sources/"running.slow","long-running-source");
    AssetOperationId running{};
    Require(database.RegisterImporter({"test.cooperative",1,AssetType::Material,{".slow"},
        [&](const AssetImportContext &context,std::string &importError)->std::optional<AssetImportOutput>{
            Require(database.CancelOperation(running),"A running importer must accept cancellation requests.");
            if(context.isCancelled()){importError="Import cancelled.";return std::nullopt;}
            return std::nullopt;
        }},&error),"A project importer should register.");
    running=database.QueueImport({sources/"running.slow"});
    Require(!database.ProcessOperation(running)&&database.GetOperations().back().state==AssetOperationState::Cancelled,
            "Running import cancellation must propagate through the cooperative importer context.");
    Write(sources/"unsupported.xyz","bad");
    Require(!database.ImportNow({sources/"unsupported.xyz"},&error) &&
            database.GetOperations().back().state==AssetOperationState::Failed && !error.empty(),
            "Importer failures should remain visible with an error.");
    Write(sources/"invalid.png","not-a-png");
    Require(!database.ImportNow({sources/"invalid.png"},&error)&&
            database.GetOperations().back().state==AssetOperationState::Failed,
            "A built-in importer must reject content that does not match its declared format.");
    Require(database.Validate().empty(),"A complete asset graph should validate.");

    AssetDatabase reloaded;
    Require(reloaded.Open(root,&error) && reloaded.GetAssets().size()==database.GetAssets().size(),
            "The asset database should persist and reload.");
    Require(reloaded.Find(*partId) && reloaded.Find(*partId)->part==part &&
            reloaded.Find(textureId)->id==textureId,
            "Stable IDs and generic part metadata should round-trip.");

    const auto legacyRoot=std::filesystem::temp_directory_path()/"pipeframe_asset_database_v1_test";
    std::filesystem::remove_all(legacyRoot);
    Write(legacyRoot/"Assets/Textures/legacy.png","legacy");
    Write(legacyRoot/".pipeframe/cache/legacy/legacy.png","legacy");
    Write(legacyRoot/".pipeframe/assets.db",
          "PIPEFRAME_ASSET_DATABASE 1 2\n"
          "ASSET \"asset-00000001\" 1 0 \"Assets/Textures/legacy.png\" \".pipeframe/cache/legacy/legacy.png\" \"pipeframe.texture\" 1 1 123 \"\" 0 0\n"
          "ENDASSET\n");
    AssetDatabase migrated;
    Require(migrated.Open(legacyRoot,&error) && migrated.GetAssets().size()==1 &&
            migrated.Find("asset-00000001")->type==AssetType::Texture,
            "Version-1 metadata should migrate into the current database model.");
    Require(migrated.Save(&error),"A migrated database should save in the current format.");

    std::filesystem::remove_all(root); std::filesystem::remove_all(sources); std::filesystem::remove_all(legacyRoot);
    std::cout << "All asset database and import pipeline tests passed.\n";
}
