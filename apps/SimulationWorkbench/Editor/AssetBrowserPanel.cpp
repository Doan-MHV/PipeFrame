#include <PipeFrame/Resources/ImageData.h>
#include <PipeFrame/UI/SchemaInspector.h>
#include "AssetBrowserPanel.h"
#include <algorithm>
namespace pipeframe::editor {
using namespace pipeframe::ui;
AssetBrowserPanel::AssetBrowserPanel() :
    typeFilters{assets::AssetType::Unknown,assets::AssetType::Texture,assets::AssetType::Audio,
        assets::AssetType::Shader,assets::AssetType::Material,assets::AssetType::Model,
        assets::AssetType::Scene,assets::AssetType::Part,assets::AssetType::Prefab,assets::AssetType::Tilemap,assets::AssetType::Tileset} {}
void AssetBrowserPanel::Refresh(const assets::AssetDatabase &value,const TilemapAssetEditor *editor,bool unique,const VisualAssetEditor *visual){database=&value;paintEditor=editor;canMakeUnique=unique;visualEditor=visual;UpdateRows();}
void AssetBrowserPanel::SetOnAssign(AssetCallback callback){onAssign=std::move(callback);}
void AssetBrowserPanel::SetOnImport(ActionCallback callback){onImport=std::move(callback);}
void AssetBrowserPanel::SetOnReimport(AssetCallback callback){onReimport=std::move(callback);}
void AssetBrowserPanel::SetOnCancel(OperationCallback callback){onCancel=std::move(callback);}
const assets::AssetId &AssetBrowserPanel::GetSelectedAssetId()const{return selectedAssetId;}
std::size_t AssetBrowserPanel::GetVisibleAssetCount()const{return visibleAssets.size();}
std::optional<assets::AssetId> AssetBrowserPanel::AssetAt(const pipeframe::Vector2f point) const {
    if(const auto key=HitKeyAt(point,"asset:"))return key->substr(6);
    return std::nullopt;
}
void AssetBrowserPanel::SetSearchText(std::string text){query.text=std::move(text);UpdateRows();}

void AssetBrowserPanel::SelectCategory(assets::AssetType type){
    query.type=type==assets::AssetType::Unknown?std::nullopt:std::optional{type};
    UpdateRows();
}

void AssetBrowserPanel::Select(const std::size_t index){
    if(index>=visibleAssets.size())return; selectedAssetId=visibleAssets[index]->id; UpdateRows();
}

void AssetBrowserPanel::UpdateRows(){
    visibleAssets=database?database->Search(query):std::vector<const assets::AssetRecord *>{};
    if(!selectedAssetId.empty()&&std::ranges::none_of(visibleAssets,[&](const auto *asset){return asset->id==selectedAssetId;}))selectedAssetId.clear();
    UpdatePreview();
    UpdateStatus();
}

void AssetBrowserPanel::UpdatePreview(){
    const auto *asset=database?database->Find(selectedAssetId):nullptr;
    if(database&&query.type==assets::AssetType::Tilemap&&paintEditor&&paintEditor->Document()&&!paintEditor->Document()->Tileset().empty())asset=database->Find(paintEditor->Document()->Tileset());
    std::string key=asset?asset->id+":"+std::to_string(asset->revision):"";
    if(asset)for(const auto &record:database->GetAssets())key+=record.id+std::to_string(record.revision)+std::to_string(int(record.state));
    const bool draft=asset&&visualEditor&&visualEditor->IsOpen()&&visualEditor->AssetId()==asset->id;
    if(draft)for(const auto &[name,value]:visualEditor->Values())key+=name+detail::PropertyText(value);
    if(key==previewKey)return;
    previewKey=key;previewLayers.clear();previewInfo.clear();previewResources.reset();previewAtlas.reset();
    if(!asset||asset->state!=assets::AssetState::Ready)return;
    std::string error;Material2D material;std::optional<Tileset2D> atlas;
    const auto read=[&]<class T>(const assets::AssetRecord &record)->std::optional<T>{
        std::ifstream input(database->GetProjectDirectory()/record.sourcePath);return LoadVisualAsset<T>(input,error);
    };
    const assets::AssetRecord *texture=asset;
    if(asset->type==assets::AssetType::Tileset){
        atlas=read.operator()<Tileset2D>(*asset);
        if(draft){Tileset2D value;if(Tileset2D::Schema().Apply(value,visualEditor->Values(),error))atlas=value;}
        if(!atlas){previewInfo=error;return;}
        const auto *record=database->Find(atlas->material.assetId);
        if(!record||record->type!=assets::AssetType::Material){previewInfo="Choose a textured material";return;}
        auto value=read.operator()<Material2D>(*record);if(!value){previewInfo=error;return;}material=*value;
    }else if(asset->type==assets::AssetType::Material){
        auto value=read.operator()<Material2D>(*asset);if(!value){previewInfo=error;return;}material=*value;
        if(draft&&!Material2D::Schema().Apply(material,visualEditor->Values(),error)){previewInfo=error;return;}
    }else if(asset->type!=assets::AssetType::Texture)return;
    if(asset->type!=assets::AssetType::Texture)texture=database->Find(material.texture.assetId);
    Vector2u size{1,1};TextureHandle handle;
    if(texture){
        if(texture->type!=assets::AssetType::Texture||texture->state!=assets::AssetState::Ready){previewInfo="Texture unavailable";return;}
        const auto path=database->GetProjectDirectory()/texture->cachePath.parent_path()/"image.png";
        ImageData image;if(!LoadImageData(path,image,error)){previewInfo=error;return;}size=image.Size();
        previewResources=std::make_shared<GraphicsResourceService>();
        handle=previewResources->LoadTextureWithOptions(path.string(),material.smooth,true,&error);
        if(!error.empty()){previewInfo=error;return;}
    }else if(atlas||!material.texture.assetId.empty()){previewInfo="Texture unavailable";return;}
    if(atlas&&!atlas->Fits(size)){previewInfo="Atlas exceeds texture dimensions";return;}
    const float extent=float(std::max(size.x,size.y));
    const float w=float(size.x)/extent*.48f,h=float(size.y)/extent*.48f;
    previewAtlas=atlas;previewTint=material.tint;
    const auto tint=material.tint;const auto repeats=atlas?Vector2f{1,1}:material.uvScale;
    const Vertex2D a{{-w,-h},tint,{0,0}},b{{w,-h},tint,{float(size.x)*repeats.x,0}},
        c{{w,h},tint,{float(size.x)*repeats.x,float(size.y)*repeats.y}},d{{-w,h},tint,{0,float(size.y)*repeats.y}};
    previewLayers.push_back({{a,b,c,a,c,d},handle});
    if(atlas){
        std::vector<Vertex2D> borders;
        const auto rect=[&](float x,float y,float width,float height){
            const auto v=[&](float px,float py){return Vertex2D{{-w+px/size.x*2*w,-h+py/size.y*2*h},{255,210,40,255},{}};};
            auto a=v(x,y),b=v(x+width,y),c=v(x+width,y+height),d=v(x,y+height);borders.insert(borders.end(),{a,b,c,a,c,d});
        };
        // Thin triangle strips keep the preview backend-neutral.
        for(unsigned id=1;id<=atlas->columns*atlas->rows;++id){const auto r=*atlas->Region(id);
            rect(r.position.x,r.position.y,r.size.x,.5f);rect(r.position.x,r.position.y,.5f,r.size.y);
            rect(r.position.x,r.position.y+r.size.y-.5f,r.size.x,.5f);rect(r.position.x+r.size.x-.5f,r.position.y,.5f,r.size.y);
        }
        previewLayers.push_back({std::move(borders),{}});
    }
    previewInfo="Preview "+std::to_string(size.x)+" x "+std::to_string(size.y)+(atlas?" | numbered left to right, top to bottom":"");
}

void AssetBrowserPanel::UpdateStatus(){
    cancellableOperation=0; std::size_t failures=0; std::string lastError;
    if(database)for(const auto &operation:database->GetOperations()){
        if(operation.state==assets::AssetOperationState::Queued||operation.state==assets::AssetOperationState::Running)cancellableOperation=operation.id;
        if(operation.state==assets::AssetOperationState::Failed){++failures;lastError=operation.error;}
    }
    std::size_t running=0,queued=0;
    if(database)for(const auto &operation:database->GetOperations()){
        running+=operation.state==assets::AssetOperationState::Running;
        queued+=operation.state==assets::AssetOperationState::Queued;
    }
    std::string text=std::to_string(running)+" IMPORTING | "+std::to_string(queued)+" QUEUED | "+std::to_string(visibleAssets.size())+" ASSETS  |  "+std::to_string(failures)+" FAILED";
    if(!lastError.empty())text+="  |  "+lastError.substr(0,40);
    statusText=std::move(text); InvalidateView();
}

View AssetBrowserPanel::BuildView() {
    std::vector<View> tabs;
    for (const auto type : typeFilters) {
        const char *label="ALL";
        switch(type) {
        case assets::AssetType::Texture: label="TEXTURES"; break;
        case assets::AssetType::Shader: label="SHADERS"; break;
        case assets::AssetType::Tilemap: label="MAPS"; break;
        case assets::AssetType::Tileset: label="TILESETS"; break;
        case assets::AssetType::Audio: label="AUDIO"; break;
        case assets::AssetType::Material: label="MATERIALS"; break;
        case assets::AssetType::Model: label="MODELS"; break;
        case assets::AssetType::Scene: label="SCENES"; break;
        case assets::AssetType::Part: label="PARTS"; break;
        case assets::AssetType::Prefab: label="PREFABS"; break;
        case assets::AssetType::Unknown: break;
        }
        tabs.push_back(views::Button(std::string("category:")+assets::ToString(type),label,
            [this,type]{SelectCategory(type);}).Selected(query.type.value_or(assets::AssetType::Unknown)==type));
    }
    std::vector<View> rows;
    for (std::size_t i=0;i<visibleAssets.size();++i) {
        const auto &asset=*visibleAssets[i];
        rows.push_back(views::Button("asset:"+asset.id,asset.sourcePath.filename().string()+" | "+assets::ToString(asset.type)+" | "+assets::ToString(asset.state),
            [this,i]{Select(i);}).Selected(asset.id==selectedAssetId).Leading());
    }
    if(rows.empty())rows.push_back(views::Text("empty","No assets match this category and search.").FitHeight());
    std::vector<View> mapTools;
    if(query.type==assets::AssetType::Tilemap)mapTools={
        views::Wrap("map-actions",{
            views::Button("new-map","NEW MAP",[this]{if(onCreateMap)onCreateMap();}),
            views::Button("edit-map","EDIT MAP",[this]{if(onEditMap)onEditMap(selectedAssetId);}).Enabled(!selectedAssetId.empty()),
            views::Button("unique-map","MAKE UNIQUE",[this]{if(onMakeUnique)onMakeUnique();})
                .Enabled(canMakeUnique),
            views::Button("stop-map","STOP PAINT",[this]{if(onStopMap)onStopMap();})}),
        views::Wrap("paint-shapes",{
            views::Button("pencil","PENCIL",[this]{paintShape=0;ApplyPaintSettings();}).Selected(paintShape==0),
            views::Button("line","LINE",[this]{paintShape=1;ApplyPaintSettings();}).Selected(paintShape==1),
            views::Button("rectangle","RECTANGLE",[this]{paintShape=2;ApplyPaintSettings();}).Selected(paintShape==2),
            views::Button("outline","OUTLINE",[this]{paintShape=3;ApplyPaintSettings();}).Selected(paintShape==3),
            views::Button("fill","FILL",[this]{paintShape=4;ApplyPaintSettings();}).Selected(paintShape==4),
            views::Button("erase","ERASE",[this]{erasing=!erasing;ApplyPaintSettings();}).Selected(erasing),
            views::Button("circle","CIRCLE",[this]{paintShape=5;ApplyPaintSettings();}).Selected(paintShape==5),
            views::Button("pick","EYEDROPPER",[this]{paintShape=6;ApplyPaintSettings();}).Selected(paintShape==6).Enabled(!paintEditor||!paintEditor->ActiveBrush()),
            views::Button("select","SELECT",[this]{paintShape=8;ApplyPaintSettings();}).Selected(paintShape==8),
            views::Button("ruler","RULER",[this]{paintShape=7;ApplyPaintSettings();}).Selected(paintShape==7)})
    };
    if(query.type==assets::AssetType::Material||query.type==assets::AssetType::Tileset){
        mapTools.push_back(views::Wrap("visual-actions",{
            views::Button("new-visual","NEW ASSET",[this]{if(visualActions.create)visualActions.create(*query.type);}),
            views::Button("edit-visual","EDIT SELECTED",[this]{if(visualActions.open)visualActions.open(selectedAssetId);}).Enabled(!selectedAssetId.empty())}));
        if(visualEditor&&visualEditor->IsOpen()){
            auto schema=visualEditor->Schema();auto values=visualEditor->Values();
            std::vector<View> references;
            for(const auto &field:schema.properties)if(field.kind==PropertyKind::AssetReference){
                std::vector<View> choices{views::Text("label",field.displayName),views::Button("none","NONE",[this,key=field.key]{if(visualActions.set)visualActions.set(key,AssetReference{});})};
                for(const auto &record:database->GetAssets())if(field.editorHint=="asset:"+std::string(assets::ToString(record.type))&&record.state==assets::AssetState::Ready)
                    choices.push_back(views::Button(record.id,record.sourcePath.filename().string(),[this,key=field.key,id=record.id]{if(visualActions.set)visualActions.set(key,AssetReference{id});})
                        .Selected(std::get<AssetReference>(values.at(field.key)).assetId==record.id));
                references.push_back(views::Scroll(field.key,views::Column("choices",std::move(choices))).Height(120));
            }
            std::erase_if(schema.properties,[](const auto &field){return field.kind==PropertyKind::AssetReference;});
            mapTools.push_back(views::Column("references",std::move(references)));
            mapTools.push_back(SchemaInspector("visual:"+visualEditor->AssetId(),schema,values,[this](const auto &key,const auto &value){fieldError.clear();if(visualActions.set)visualActions.set(key,value);},[this](const auto &,const auto &message){fieldError=message;InvalidateView();}));
            mapTools.push_back(views::Wrap("visual-save",{
                views::Button("save","SAVE ASSET",[this]{if(visualActions.save)visualActions.save();}),
                views::Button("undo","UNDO",[this]{if(visualActions.undo)visualActions.undo();}).Enabled(visualEditor->CanUndo()),
                views::Button("redo","REDO",[this]{if(visualActions.redo)visualActions.redo();}).Enabled(visualEditor->CanRedo())}));
            mapTools.push_back(views::Text("visual-error",visualEditor->LastError().empty()?(visualEditor->IsDirty()?"ASSET UNSAVED":"ASSET SAVED"):visualEditor->LastError()).FitHeight());
        }
    }
    if(query.type==assets::AssetType::Tilemap && paintEditor && paintEditor->Document()) {
        std::vector<View> brushChoices{views::Button("tiles","VISUAL TILES",[this]{if(environmentActions.selectBrush)environmentActions.selectBrush("");})};
        for(const auto &brush:paintEditor->Brushes())brushChoices.push_back(views::Button(brush.id,brush.category+" / "+brush.label,[this,id=brush.id]{erasing=false;if(paintTile==0)paintTile=1;if(environmentActions.selectBrush)environmentActions.selectBrush(id);}));
        mapTools.push_back(views::Wrap("project-brushes",std::move(brushChoices)));
        if(const auto *brush=paintEditor->ActiveBrush()){
            mapTools.push_back(SchemaInspector("brush:"+brush->SettingsSchema().typeId,brush->SettingsSchema(),brush->Settings(),
                [this](const auto &key,const auto &value){fieldError.clear();if(environmentActions.brushSetting)environmentActions.brushSetting(key,value);},
                [this](const auto &,const auto &message){fieldError=message;InvalidateView();}));
            mapTools.push_back(views::Text("brush-layer","DATA LAYER: "+std::string(brush->DataTarget())).FitHeight());
        }
        mapTools.push_back(views::Text("paint-target","TARGET: "+paintTarget+(paintEditor->IsEnabled()?" | EDITING":" | STOPPED")).FitHeight());
        mapTools.push_back(views::Text("paint-help","Cells snap automatically. L toggles horizontal/vertical axis lock. Escape cancels. Middle drag pans; wheel zooms.").FitHeight());
        mapTools.push_back(views::NumberField("brush-radius","Pencil / eraser radius (cells)",paintEditor->BrushRadius(),[this](float v){
            if(std::isfinite(v)&&v==std::floor(v)&&v>=0&&v<=64){if(environmentActions.brush)environmentActions.brush(int(v),paintEditor->AxisConstraint());}
            else {fieldError="Radius must be an integer from 0 to 64";InvalidateView();}
        }));
        mapTools.push_back(views::Toggle("axis-lock","AXIS LOCK",paintEditor->AxisConstraint(),[this](bool v){if(environmentActions.brush)environmentActions.brush(paintEditor->BrushRadius(),v);}));
        mapTools.push_back(views::Wrap("map-history",{
            views::Button("map-save","SAVE MAP",[this]{if(environmentActions.save)environmentActions.save();}),
            views::Button("map-undo","UNDO MAP",[this]{if(environmentActions.undo)environmentActions.undo();}).Enabled(paintEditor->CanUndo()),
            views::Button("map-redo","REDO MAP",[this]{if(environmentActions.redo)environmentActions.redo();}).Enabled(paintEditor->CanRedo())}));
        if(auto selection=paintEditor->Selection()){
            mapTools.push_back(views::Text("selection-size",std::to_string(selection->maximum.column-selection->minimum.column+1)+" x "+std::to_string(selection->maximum.row-selection->minimum.row+1)+" selected cells"));
            mapTools.push_back(views::Wrap("selection-actions",{
                views::Button("fill-selection","PAINT SELECTION",[this]{if(environmentActions.selection)environmentActions.selection(paintTile,false);}),
                views::Button("erase-selection","ERASE SELECTION",[this]{if(environmentActions.selection)environmentActions.selection(0,false);})}));
        }
        mapTools.push_back(views::Button("boundary","PAINT BOUNDARY",[this]{if(environmentActions.selection)environmentActions.selection(paintTile,true);}));
        std::vector<View> layers,palette;
        const auto *map=paintEditor->Document();
        paintLayer=std::min(paintEditor->ActiveLayer(),map->Layers().size()-1);
        if(sizingAsset!=paintEditor->AssetId()){sizingAsset=paintEditor->AssetId();resizeColumns=map->Columns();resizeRows=map->Rows();resizeCellSize=map->CellSize();}
        mapTools.push_back(views::NumberField("resize-columns","Columns",resizeColumns,[this](float v){if(std::isfinite(v)&&v>=1&&v<=2048)resizeColumns=v;}));
        mapTools.push_back(views::NumberField("resize-rows","Rows",resizeRows,[this](float v){if(std::isfinite(v)&&v>=1&&v<=2048)resizeRows=v;}));
        mapTools.push_back(views::NumberField("resize-cell","Cell Size",resizeCellSize,[this](float v){if(std::isfinite(v)&&v>0&&v<=1000)resizeCellSize=v;}));
        mapTools.push_back(views::Wrap("resize-actions",{
            views::Button("preview-resize","PREVIEW RESIZE",[this]{if(visualActions.previewResize)visualActions.previewResize(int(resizeColumns),int(resizeRows),resizeCellSize,false);}),
            views::Button("preview-resample","RESAMPLE SAME AREA",[this]{if(visualActions.previewResize)visualActions.previewResize(int(resizeColumns),int(resizeRows),resizeCellSize,true);}),
            views::Button("apply-resize","APPLY SIZE",[this]{if(visualActions.applyResize)visualActions.applyResize();})}));
        if(!paintEditor->ResizeSummary().empty())mapTools.push_back(views::Text("resize-preview",paintEditor->ResizeSummary()).FitHeight());
        std::vector<View> atlasChoices{views::Text("tileset-label","Map Tileset")};
        for(const auto &record:database->GetAssets())if(record.type==assets::AssetType::Tileset&&record.state==assets::AssetState::Ready)
            atlasChoices.push_back(views::Button(record.id,record.sourcePath.filename().string(),[this,id=record.id]{if(visualActions.tileset)visualActions.tileset(id);}).Selected(map->Tileset()==record.id));
        mapTools.push_back(views::Scroll("tileset-choices",views::Column("choices",std::move(atlasChoices))).Height(100));
        if(paintShape==6 && paintEditor->SampledTile()){paintTile=*paintEditor->SampledTile();erasing=false;}
        if(auto measurement=paintEditor->Measurement()){
            const double dx=double(measurement->second.column)-measurement->first.column;
            const double dy=double(measurement->second.row)-measurement->first.row;
            mapTools.push_back(views::Text("measurement",std::to_string(std::hypot(dx*paintScale.x,dy*paintScale.y)*map->CellSize())+" world units | "+std::to_string(std::max(std::abs(dx),std::abs(dy))+1)+" cells").FitHeight());
        }
        if(auto gesture=paintEditor->Gesture())mapTools.push_back(views::Text("gesture-size",
            "From ("+std::to_string(gesture->first.column)+", "+std::to_string(gesture->first.row)+") to ("+
            std::to_string(gesture->second.column)+", "+std::to_string(gesture->second.row)+") | "+
            std::to_string(std::abs(gesture->second.column-gesture->first.column)+1)+" x "+std::to_string(std::abs(gesture->second.row-gesture->first.row)+1)+" cells").FitHeight());
        for(std::size_t i=0;i<map->Layers().size();++i){
            const auto &layer=map->Layers()[i];
            layers.push_back(views::Button("layer:"+std::to_string(i),std::to_string(i)+": "+layer.name+(layer.locked?" [LOCKED]":"")+(!layer.visible?" [HIDDEN]":""),
                [this,i]{paintLayer=i;ApplyPaintSettings();}).Selected(paintLayer==i));
        }
        const auto &active=map->Layers()[paintLayer];
        mapTools.push_back(views::Text("layers-order","LAYERS (later layers draw on top)"));
        mapTools.push_back(views::Wrap("layers",std::move(layers)));
        mapTools.push_back(views::TextField("layer-name","Layer Name",active.name,[this](const std::string &name){if(environmentActions.layer)environmentActions.layer("rename",paintLayer,name);}));
        std::vector<View> operations;
        for(const auto &[action,label]:std::vector<std::pair<std::string,std::string>>{{"add","ADD LAYER"},{"remove","REMOVE LAYER"},{"up","MOVE UP"},{"down","MOVE DOWN"},
            {"visible",active.visible?"HIDE":"SHOW"},{"collision",active.collision?"COLLISION ON":"COLLISION OFF"},{"lock",active.locked?"UNLOCK":"LOCK"}})
            operations.push_back(views::Button(action,label,[this,action]{if(environmentActions.layer)environmentActions.layer(action,paintLayer,{});})
                .Enabled(action=="remove"?map->Layers().size()>1:action=="up"?paintLayer+1<map->Layers().size():action=="down"?paintLayer>0:true));
        mapTools.push_back(views::Wrap("layer-operations",std::move(operations)));
        std::vector<TileId> ids;for(const auto &[id,definition]:map->Definitions())ids.push_back(id);
        std::ranges::sort(ids);
        const std::size_t pages=std::max<std::size_t>(1,(ids.size()+31)/32);palettePage=std::min(palettePage,pages-1);
        for(std::size_t index=palettePage*32;index<std::min(ids.size(),(palettePage+1)*32);++index){
            const auto id=ids[index];const auto &tile=*map->Definition(id);auto color=tile.tint;
            Vector2f uv{},extent{};TextureHandle texture;
            if(previewAtlas&&!map->Tileset().empty()&&!previewLayers.empty())if(auto region=previewAtlas->Region(id)){
                uv=region->position;extent=region->size;texture=previewLayers.front().texture;
                color={std::uint8_t(unsigned(color.r)*previewTint.r/255),std::uint8_t(unsigned(color.g)*previewTint.g/255),std::uint8_t(unsigned(color.b)*previewTint.b/255),std::uint8_t(unsigned(color.a)*previewTint.a/255)};
            }
            const Vertex2D a{{-.45f,-.45f},color,uv},b{{.45f,-.45f},color,uv+Vector2f{extent.x,0}},c{{.45f,.45f},color,uv+extent},d{{-.45f,.45f},color,uv+Vector2f{0,extent.y}};
            palette.push_back(views::Column("tile:"+std::to_string(id),{
                views::Mesh("swatch",{{{a,b,c,a,c,d},texture}},previewResources).Height(32),
                views::Button("choose","TILE "+std::to_string(id)+(tile.solid?" WALL":""),[this,id]{paintTile=id;erasing=false;ApplyPaintSettings();}).Selected(paintTile==id)}));
        }
        mapTools.push_back(views::Text("palette-title","TILES | PAGE "+std::to_string(palettePage+1)+" / "+std::to_string(pages)));
        mapTools.push_back(views::Wrap("palette",std::move(palette),85));
        if(pages>1)mapTools.push_back(views::Row("palette-pages",{
            views::Button("previous","PREVIOUS",[this]{if(palettePage)--palettePage;InvalidateView();}).Enabled(palettePage>0),
            views::Button("next","NEXT",[this]{++palettePage;InvalidateView();}).Enabled(palettePage+1<pages)}));
        if(map->Tileset().empty())mapTools.push_back(views::Button("new-tile","NEW COLOR TILE",[this]{
            TileId id=1;while(paintEditor->Document()->Definition(id))++id;
            if(environmentActions.tile)environmentActions.tile(id,{255,255,255,255},false);paintTile=id;
        }));
        if(const auto *tile=map->Definition(paintTile)){
            mapTools.push_back(views::TextField("tile-color","Tile Tint (r, g, b, a)",detail::PropertyText(tile->tint),[this](const std::string &text){
                try {auto color=std::get<Color>(detail::ParseProperty(PropertyKind::Color,text));if(environmentActions.tile)environmentActions.tile(paintTile,color,paintEditor->Document()->Definition(paintTile)->solid);fieldError.clear();}
                catch(const std::exception &e){fieldError=e.what();InvalidateView();}
            }));
            mapTools.push_back(views::Toggle("tile-solid","SOLID TILE",tile->solid,[this](bool v){if(environmentActions.tile)environmentActions.tile(paintTile,paintEditor->Document()->Definition(paintTile)->tint,v);}));
        }
        mapTools.push_back(views::Text("paint-state",paintEditor->LastError().empty()?
            (paintEditor->IsDirty()?"MAP UNSAVED":"MAP SAVED"):paintEditor->LastError()).FitHeight());
    }
    if(!fieldError.empty())mapTools.push_back(views::Text("field-error",fieldError).FitHeight());
    if(visualEditor&&!visualEditor->LastError().empty()&&!visualEditor->IsOpen())mapTools.push_back(views::Text("open-error",visualEditor->LastError()).FitHeight());
    std::vector<View> assignments;
    const auto *selected=database?database->Find(selectedAssetId):nullptr;
    if(selected&&selected->state==assets::AssetState::Ready)for(const auto &target:assignmentTargets){
        if(!target.hint.empty()&&target.hint!="asset:"+std::string(assets::ToString(selected->type)))continue;
        assignments.push_back(views::Button(target.component+":"+target.key,"ASSIGN TO "+target.label,[this,target]{if(onAssignProperty)onAssignProperty(target.component,target.key,selectedAssetId);}));
    }
    std::vector<View> preview;
    if(!previewLayers.empty())preview.push_back(views::Mesh("texture-preview",previewLayers,previewResources).Height(96));
    if(!previewInfo.empty())preview.push_back(views::Text("preview-info",previewInfo).FitHeight());
    auto list=views::Scroll("list",views::Column("rows",std::move(rows)));
    if(query.type==assets::AssetType::Tilemap||query.type==assets::AssetType::Material||query.type==assets::AssetType::Tileset)list.Height(140);else list.Expanded();
    auto content=views::Column("assets",{
        views::Text("title","ASSET BROWSER"),
        views::TextField("search","Search assets",query.text,[this](const std::string &text){SetSearchText(text);}),
        views::Wrap("categories",std::move(tabs),90),
        std::move(list),
        views::Wrap("actions",{
            views::Button("import","IMPORT",[this]{if(onImport)onImport();}),
            views::Button("assign","INSTANTIATE PREFAB",[this]{if(onAssign)onAssign(selectedAssetId);}).Enabled(selected&&selected->type==assets::AssetType::Prefab),
            views::Button("reimport","REIMPORT",[this]{if(onReimport)onReimport(selectedAssetId);}).Enabled(!selectedAssetId.empty()),
            views::Button("cancel","CANCEL",[this]{if(onCancel)onCancel(cancellableOperation);}).Enabled(cancellableOperation!=0)}),
        views::Wrap("assignment-targets",std::move(assignments)),
        views::Column("map-tools",std::move(mapTools)),
        views::Column("asset-preview",std::move(preview)),
        views::Text("status",statusText).FitHeight()
    }).Padding(10);
    if(query.type==assets::AssetType::Tilemap||query.type==assets::AssetType::Material||query.type==assets::AssetType::Tileset)return views::Scroll("map-page",content.FitHeight()).FillHeight();
    return content.FillHeight();
}
}
