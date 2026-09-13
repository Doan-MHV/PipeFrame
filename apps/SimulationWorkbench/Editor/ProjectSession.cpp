#include <PipeFrame/Render/RenderContext.h>
#include <PipeFrame/Environment/TilemapGeometry.h>
#include <PipeFrame/Environment/PlaygroundGeometry.h>
#include <PipeFrame/Components/TilemapComponent.h>
#include <fstream>
#include "ProjectSession.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <utility>

namespace pipeframe::editor {

namespace {
const PropertyDescriptor *FindPropertyDescriptor(
    const std::vector<PropertyDescriptor> &properties, const std::string &key) {
    const auto found = std::ranges::find(properties, key, &PropertyDescriptor::key);
    return found == properties.end() ? nullptr : &*found;
}

const SceneComponentData *FindComponent(const SceneObjectData &object, const std::string &typeId) {
    const auto found = std::ranges::find(object.components, typeId, &SceneComponentData::typeId);
    return found == object.components.end() ? nullptr : &*found;
}

bool IsSafePrefabId(const std::string &id) {
    return !id.empty() && std::ranges::all_of(id, [](const unsigned char value) {
        return std::isalnum(value) || value == '-' || value == '_' || value == '.';
    });
}

std::filesystem::path PrefabPath(const std::filesystem::path &projectDirectory,
                                 const std::string &id) {
    return projectDirectory / "Assets" / "Prefabs" / (id + ".pfprefab");
}
}

ProjectSession::~ProjectSession(){runtimeHost.Unload();runtimeHost.SetBrushLifecycle({},{});}

ProjectSession::ProjectSession(
    std::filesystem::path defaultProjectsDirectory)
    : defaultProjectsDirectory(
          std::move(defaultProjectsDirectory)),
      projectManager(
          this->defaultProjectsDirectory /
          ".recent-projects") {
    runtimeHost.SetAssetDatabase(assetDatabase);
    runtimeHost.SetBrushLifecycle([this]{tilemapEditor.ClearBrushes();},[this]{tilemapEditor.SetBrushes(runtimeHost.EnvironmentBrushes());});
    commonComponentTypes = {
        {Transform2DComponentTypeId,"Transform",1,false,false,
         {{"position","Position",PropertyKind::Vector2,Vector2f{},true,"world units"},
          {"rotation","Rotation",PropertyKind::Number,0.0,true,"degrees"},
          {"scale","Scale",PropertyKind::Vector2,Vector2f{1,1},true}}},
        {"pipeframe.identity","Identity",1,false,false,
         {{"label","Label",PropertyKind::String,std::string{},true}}},
        {"pipeframe.physics-body2d","Physics Body 2D",1,true,false,
         {{"bodyType","Body Type",PropertyKind::String,std::string{"dynamic"},true},
          {"mass","Mass",PropertyKind::Number,1.0,true,"kg",0.001},
          {"damping","Damping",PropertyKind::Number,0.0,true,"",0.0,1.0}}},
        {"pipeframe.collider2d","Collider 2D",1,true,false,
         {{"shape","Shape",PropertyKind::String,std::string{"box"},true},
          {"size","Size",PropertyKind::Vector2,Vector2f{1,1},true,"world units"},
          {"trigger","Is Trigger",PropertyKind::Boolean,false,true}}},
        {"pipeframe.renderer2d","Renderer 2D",1,true,false,
         {{"tint","Tint",PropertyKind::Color,Color{255,255,255,255},true},
          {"texture","Texture",PropertyKind::AssetReference,AssetReference{},true},
          {"sortingOrder","Sorting Order",PropertyKind::Integer,std::int64_t{0},true}}}
    };
}

bool ProjectSession::OpenStartupProject(
    const std::optional<std::filesystem::path>
        &startupProject,
    std::string *errorMessage) {

    if (!startupProject.has_value()) {
        return true;
    }

    return OpenProject(
        *startupProject,
        errorMessage);
}

bool ProjectSession::CreateProjectAt(
    const std::filesystem::path &directory,
    std::string *errorMessage) {

    if (!CanSwitchProject(errorMessage)) {
        return false;
    }

    if (directory.empty()) {
        SetError(
            errorMessage,
            "Project directory cannot be empty.");

        return false;
    }

    SceneDocument newDocument;
    newDocument.MarkClean();

    std::string projectName =
        directory.filename().string();

    if (projectName.empty()) {
        projectName = "Untitled";
    }

    if (!projectManager.CreateProject(
            directory,
            projectName,
            newDocument,
            errorMessage)) {

        return false;
            }

    ActivateDocument(
        std::move(newDocument));

    return true;
}

bool ProjectSession::CreateNewProject(
    std::string *errorMessage) {

    if (!CanSwitchProject(errorMessage)) {
        return false;
    }

    SceneDocument newDocument;
    newDocument.MarkClean();

    const std::filesystem::path directory =
        ProjectManager::FindAvailableProjectDirectory(
            defaultProjectsDirectory,
            "Untitled");

    if (!projectManager.CreateProject(
            directory,
            directory.filename().string(),
            newDocument,
            errorMessage)) {

        return false;
    }

    ActivateDocument(std::move(newDocument));
    return true;
}

bool ProjectSession::OpenNextRecentProject(
    std::string *errorMessage) {

    if (!CanSwitchProject(errorMessage)) {
        return false;
    }

    const std::filesystem::path currentManifest =
        projectManager
            .GetActiveManifestPath()
            .lexically_normal();

    for (const std::filesystem::path
             &recentProject :
         projectManager.GetRecentProjects()) {

        if (recentProject.lexically_normal() ==
            currentManifest) {

            continue;
        }

        std::string openError;

        if (OpenProject(
                recentProject,
                &openError)) {

            return true;
        }

        std::cerr << openError << '\n';
    }

    SetError(
        errorMessage,
        "No other recent PipeFrame project is available.");

    return false;
}

std::vector<std::filesystem::path>
ProjectSession::GetRecentProjects() const {
    return projectManager.GetRecentProjects();
}

bool ProjectSession::BeginTilemapEditing(const std::string &id) {
    if(!tilemapEditor.Open(assetDatabase,id))return false;
    tilemapEditor.SetBrushes(runtimeHost.EnvironmentBrushes());
    ServiceRegistry services;services.Provide(assetDatabase);ProjectRuntimeContext context;context.services=&services;
    std::string error;editingVisuals.Load(context,error);tilemapAtlas.reset();
    tilemapTransform={};tilemapTarget.reset();tilemapBounds.reset();
    if(const auto *object=GetSelectedObject())for(const auto &component:object->components)
        if(component.typeId==TilemapComponentTypeId){
            const auto found=component.properties.find("asset");
            if(found!=component.properties.end())if(const auto *reference=std::get_if<AssetReference>(&found->second);
                reference&&reference->assetId==id){tilemapTransform=object->transform;tilemapTarget=object->id;}
        }
    tilemapGeometryRevision=0;dataOverlayRevision=0;dataOverlayCells.clear();
    const auto *map=tilemapEditor.Document();
    TileId tile=0;
    for(const auto &[id,definition]:map->Definitions())if(!tile||id<tile)tile=id;
    tilemapEditor.Configure(0,std::make_shared<TileBrush>(tile));
    if(!RefreshTilemapTarget())return false;
    tilemapEditor.SetEnabled(true);return true;
}

bool ProjectSession::CanMakeSelectedTilemapUnique() const {
    if(!tilemapEditor.Document() || selectedObjectIds.size()!=1 || tilemapEditor.IsDirty() || tilemapEditor.HasPointerCapture())return false;
    const auto *object=GetSelectedObject();
    if(!object || object->locked || !object->visible)return false;
    const auto *component=FindComponent(*object,TilemapComponentTypeId);
    if(!component)return false;
    const auto field=component->properties.find("asset");
    if(field==component->properties.end())return false;
    const auto *reference=std::get_if<AssetReference>(&field->second);
    return reference && reference->assetId==tilemapEditor.AssetId();
}

bool ProjectSession::MakeSelectedTilemapUnique() {
    if(!CanMakeSelectedTilemapUnique())return false;
    const auto copied=tilemapEditor.CopyAsset();
    if(!copied)return false;
    if(!AssignAssetToSelectedProperty(TilemapComponentTypeId,"asset",*copied))return false;
    // Clean document only: opening the copy cannot discard a pending stroke or edits.
    return BeginTilemapEditing(*copied);
}

bool ProjectSession::CreateTilemapAsset() {
    if(assetDatabase.HasActiveImport())return false;
    if(!tilemapEditor.Close())return false;
    const auto folder=assetDatabase.GetProjectDirectory()/"Assets/Tilemaps";
    std::error_code ec;std::filesystem::create_directories(folder,ec);if(ec)return false;
    auto path=folder/"Environment.pftilemap";
    for(int n=2;std::filesystem::exists(path);++n)path=folder/("Environment"+std::to_string(n)+".pftilemap");
    int columns=32,rows=24;float cellSize=10;
    if(const auto *object=GetSelectedObject())for(const auto &component:object->components)
        if(component.typeId==PlaygroundComponentTypeId){
            if(auto it=component.properties.find("columns");it!=component.properties.end())if(auto *v=std::get_if<std::int64_t>(&it->second))columns=int(*v);
            if(auto it=component.properties.find("rows");it!=component.properties.end())if(auto *v=std::get_if<std::int64_t>(&it->second))rows=int(*v);
            if(auto it=component.properties.find("cellSize");it!=component.properties.end())if(auto *v=std::get_if<double>(&it->second))cellSize=float(*v);
        }
    Tilemap2D map(columns,rows,cellSize);map.DefineTile({1,{90,100,110,255},true});map.AddLayer("Walls");
    {std::ofstream output(path);if(!TilemapSerializer::Save(map,output))return false;}
    const auto id=assetDatabase.ImportNow({path});
    if(!id)return false;
    AssignAssetToFirstSelectedAssetProperty(*id);
    return BeginTilemapEditing(*id);
}

bool ProjectSession::RefreshTilemapTarget() {
    if(!tilemapTarget)return true;
    if(selectedObjectIds.size()!=1||selectedObjectId!=tilemapTarget){tilemapEditor.SetEnabled(false);return false;}
    const auto *object=document.FindObject(*tilemapTarget);
    bool matches=false;
    if(object&&!object->locked&&object->visible)for(const auto &component:object->components)
        if(component.typeId==TilemapComponentTypeId){
            const auto it=component.properties.find("asset");
            if(it!=component.properties.end())if(const auto *reference=std::get_if<AssetReference>(&it->second))
                matches=reference->assetId==tilemapEditor.AssetId();
        }
    if(!matches){tilemapEditor.SetEnabled(false);return false;}
    std::optional<Rectanglef> bounds;
    if(const auto *component=FindComponent(*object,PlaygroundComponentTypeId)) {
        PlaygroundComponent ground;std::string error;
        if(!PlaygroundComponent::Schema().Apply(ground,component->properties,error)){
            tilemapEditor.SetEnabled(false);return false;
        }
        bounds=Rectanglef{{},ground.Size()};
    }
    if(bounds!=tilemapBounds){
        tilemapBounds=bounds;tilemapEditor.SetBounds(bounds);tilemapGeometryRevision=0;
    }
    if(tilemapTransform!=object->transform){
        tilemapEditor.SetEnabled(false);tilemapTransform=object->transform;tilemapGeometryRevision=0;
        return false;
    }
    return true;
}

bool ProjectSession::HandleTilemapEvent(const InputEvent &event,RenderContext &context) {
    if(!RefreshTilemapTarget()||!tilemapEditor.IsEnabled())return false;
    std::optional<Vector2i> pixel;
    if(auto *p=event.GetIf<PointerInput>())pixel=p->position;
    if(auto *p=event.GetIf<PointerMoveInput>())pixel=p->position;
    std::optional<GridCoordinate> cell;
    bool insideBounds=true;
    if(pixel && tilemapTransform.scale.x!=0 && tilemapTransform.scale.y!=0){
        auto point=context.MapPixelToWorld(*pixel)-tilemapTransform.position;
        const float c=std::cos(tilemapTransform.rotation),s=std::sin(tilemapTransform.rotation);
        point={(point.x*c+point.y*s)/tilemapTransform.scale.x,(-point.x*s+point.y*c)/tilemapTransform.scale.y};
        if(tilemapBounds)insideBounds=point.x>=tilemapBounds->position.x&&point.y>=tilemapBounds->position.y&&
            point.x<tilemapBounds->position.x+tilemapBounds->size.x&&point.y<tilemapBounds->position.y+tilemapBounds->size.y;
        const auto *map=tilemapEditor.Document();point=(point-map->Origin())/map->CellSize();
        if(std::isfinite(point.x)&&std::isfinite(point.y))cell=GridCoordinate{
            int(std::clamp(std::floor(double(point.x)),-1000000.0,1000000.0)),
            int(std::clamp(std::floor(double(point.y)),-1000000.0,1000000.0))};
    }
    return tilemapEditor.HandleEvent(event,cell,insideBounds&&pixel&&context.IsInsideWorldViewport(*pixel));
}

void ProjectSession::RenderTilemapEditing(RenderContext &context) {
    const auto *map=tilemapEditor.Document();if(!map||!RefreshTilemapTarget()||!tilemapEditor.IsEnabled())return;
    Transform2DComponent transform;transform.position=tilemapTransform.position;
    transform.rotation=tilemapTransform.rotation;transform.scale=tilemapTransform.scale;
    auto canvas=context.GetCanvas();
    std::vector<Vertex2D> overlays;bool collect=false;
    const auto emit=[&](Rectanglef bounds,Color color){
        if(tilemapBounds){
            const auto right=std::min(bounds.position.x+bounds.size.x,tilemapBounds->position.x+tilemapBounds->size.x);
            const auto bottom=std::min(bounds.position.y+bounds.size.y,tilemapBounds->position.y+tilemapBounds->size.y);
            bounds.position.x=std::max(bounds.position.x,tilemapBounds->position.x);
            bounds.position.y=std::max(bounds.position.y,tilemapBounds->position.y);
            bounds.size={right-bounds.position.x,bottom-bounds.position.y};
            if(bounds.size.x<=0||bounds.size.y<=0)return;
        }
        const auto a=PlaygroundToWorld(bounds.position,transform);
        const auto b=PlaygroundToWorld(bounds.position+Vector2f{bounds.size.x,0},transform);
        const auto c=PlaygroundToWorld(bounds.position+bounds.size,transform);
        const auto d=PlaygroundToWorld(bounds.position+Vector2f{0,bounds.size.y},transform);
        const Vertex2D vertices[]{{a,color,{}},{b,color,{}},{c,color,{}},{a,color,{}},{c,color,{}},{d,color,{}}};
        if(collect)overlays.insert(overlays.end(),std::begin(vertices),std::end(vertices));else canvas.Draw(vertices,6,PrimitiveTopology::Triangles);
    };
    emit(map->Bounds(),{30,35,40,255});
    const VisualAssetModule::MaterialResource *material=nullptr;
    std::optional<Tileset2D> atlas;
    if(!map->Tileset().empty()){
        const auto *resolved=editingVisuals.ResolveTileset({map->Tileset()});
        if(!resolved||!resolved->error.empty())return;
        atlas=resolved->value;material=editingVisuals.ResolveMaterial(atlas->material);
    }
    const auto tint=material?material->value.tint:Color{255,255,255,255};
    if(tilemapAtlas!=atlas||tilemapTint!=tint){tilemapAtlas=atlas;tilemapTint=tint;tilemapGeometryRevision=0;}
    if(tilemapGeometryRevision!=map->Revision()){
        tilemapChunks.Update(*map,atlas?&*atlas:nullptr,true);
        const auto bounds=map->Bounds();
        const bool fullyVisible=!tilemapBounds || (tilemapBounds->position.x<=bounds.position.x && tilemapBounds->position.y<=bounds.position.y &&
            tilemapBounds->position.x+tilemapBounds->size.x>=bounds.position.x+bounds.size.x && tilemapBounds->position.y+tilemapBounds->size.y>=bounds.position.y+bounds.size.y);
        if(fullyVisible)tilemapChunks.Flatten(tilemapVertices);
        else {auto geometry=tilemapChunks.Flatten();ClipTilemapGeometry(geometry.vertices,*tilemapBounds,tilemapVertices);}
        for(auto &vertex:tilemapVertices){vertex.position=PlaygroundToWorld(vertex.position,transform);vertex.color=MultiplyTint(vertex.color,tint);}
        tilemapGeometryRevision=map->Revision();
    }
    canvas.Draw(tilemapVertices.data(),tilemapVertices.size(),PrimitiveTopology::Triangles,material?material->State():RenderState{});
    if(auto measurement=tilemapEditor.Measurement()) {
        auto position=[&](GridCoordinate cell){return PlaygroundToWorld(map->Origin()+
            Vector2f{cell.column+0.5f,cell.row+0.5f}*map->CellSize(),transform);};
        const Vertex2D line[]{{position(measurement->first),{255,230,60,255},{}},
                              {position(measurement->second),{255,230,60,255},{}}};
        canvas.Draw(line,2,PrimitiveTopology::Lines);
    }
    if(const auto bounds=tilemapEditor.ResizeBounds()){
        const auto a=PlaygroundToWorld(bounds->position,transform),b=PlaygroundToWorld(bounds->position+Vector2f{bounds->size.x,0},transform),
                   c=PlaygroundToWorld(bounds->position+bounds->size,transform),d=PlaygroundToWorld(bounds->position+Vector2f{0,bounds->size.y},transform);
        const Color color{255,210,40,255};const Vertex2D lines[]{{a,color,{}},{b,color,{}},{b,color,{}},{c,color,{}},{c,color,{}},{d,color,{}},{d,color,{}},{a,color,{}}};
        canvas.Draw(lines,8,PrimitiveTopology::Lines);
    }
    collect=true;
    const auto patch=tilemapEditor.Preview();
    for(const auto &change:patch.changes)emit({map->Origin()+Vector2f{float(change.cell.column),float(change.cell.row)}*map->CellSize(),{map->CellSize(),map->CellSize()}},{30,35,40,255});
    canvas.Draw(overlays.data(),overlays.size(),PrimitiveTopology::Triangles);overlays.clear();
    auto preview=BuildTilemapPreviewGeometry(*map,patch,atlas?&*atlas:nullptr);std::vector<Vertex2D> clipped;
    if(tilemapBounds){ClipTilemapGeometry(preview.vertices,*tilemapBounds,clipped);preview.vertices=std::move(clipped);}
    for(auto &vertex:preview.vertices){vertex.position=PlaygroundToWorld(vertex.position,transform);vertex.color=MultiplyTint(vertex.color,tint);}
    canvas.Draw(preview.vertices.data(),preview.vertices.size(),PrimitiveTopology::Triangles,material?material->State():RenderState{});
    if(const auto *brush=tilemapEditor.ActiveBrush();brush&&!brush->DataTarget().empty()){
        if(const auto *data=map->DataLayer(brush->DataTarget())){
            // Data overlays are editor-only. Runtime rendering stays project-owned.
            std::map<std::pair<int,int>,double> pending;
            for(const auto &change:patch.dataChanges)pending[{change.cell.column,change.cell.row}]=change.after;
            if(dataOverlayRevision!=map->Revision()||dataOverlayTarget!=data->id){
                dataOverlayCells.clear();dataOverlayTarget=data->id;dataOverlayRevision=map->Revision();
                for(int y=0;y<map->Rows();++y)for(int x=0;x<map->Columns();++x)
                    if(const double value=data->cells.At({x,y});value!=0)dataOverlayCells.push_back({{x,y},value});
            }
            const auto cellOverlay=[&](GridCoordinate cell,double value){
                if(value!=0)emit({map->Origin()+Vector2f{float(cell.column),float(cell.row)}*map->CellSize(),{map->CellSize(),map->CellSize()}},{70,220,120,140});
            };
            for(const auto &[cell,value]:dataOverlayCells)if(!pending.contains({cell.column,cell.row}))cellOverlay(cell,value);
            for(const auto &[cell,value]:pending)cellOverlay({cell.first,cell.second},value);
        }
    }
    if(auto selection=tilemapEditor.Selection()){
        const auto p=map->Origin()+Vector2f{float(selection->minimum.column),float(selection->minimum.row)}*map->CellSize();
        const auto size=Vector2f{float(selection->maximum.column-selection->minimum.column+1),float(selection->maximum.row-selection->minimum.row+1)}*map->CellSize();
        emit({p,size},{60,170,255,60});
    }
    // Keep the pencil/eraser footprint above the stroke preview while dragging.
    if(auto hover=tilemapEditor.HoverCell();hover&&(!tilemapEditor.HasPointerCapture()||tilemapEditor.Shape()==TilemapPaintShape::Pencil)){
        const int r=tilemapEditor.Shape()==TilemapPaintShape::Pencil?tilemapEditor.BrushRadius():0;
        for(int y=-r;y<=r;++y)for(int x=-r;x<=r;++x){
            GridCoordinate cell{hover->column+x,hover->row+y};if(x*x+y*y>r*r||!map->Contains(cell))continue;
            emit({map->Origin()+Vector2f{float(cell.column),float(cell.row)}*map->CellSize(),{map->CellSize(),map->CellSize()}},{255,220,60,55});
        }
    }
    canvas.Draw(overlays.data(),overlays.size(),PrimitiveTopology::Triangles);
    if(const auto cell=tilemapEditor.HoverCell();cell&&tilemapEditor.Shape()==TilemapPaintShape::Pencil){
        const auto center=map->Origin()+Vector2f{float(cell->column)+.5f,float(cell->row)+.5f}*map->CellSize();
        const float radius=(float(tilemapEditor.BrushRadius())+.5f)*map->CellSize();
        std::vector<Vertex2D> ring;ring.reserve(192);
        for(int i=0;i<96;++i)for(int end:{i,i+1}){
            const float angle=float(end)*6.28318530718f/96.f;
            ring.push_back({PlaygroundToWorld(center+Vector2f{std::cos(angle),std::sin(angle)}*radius,transform),{255,220,60,255},{}});
        }
        canvas.Draw(ring.data(),ring.size(),PrimitiveTopology::Lines);
    }
}

bool ProjectSession::Save(
    std::string *errorMessage) {

    if(visualAssetEditor.IsOpen()&&!visualAssetEditor.Save()){SetError(errorMessage,visualAssetEditor.LastError());return false;}
    if (tilemapEditor.Document() && !tilemapEditor.Save()) {
        SetError(errorMessage,tilemapEditor.LastError()); return false;
    }
    if (!projectManager.SaveActiveScene(
            document,
            errorMessage)) {

        return false;
    }

    if (!assetDatabase.Save(errorMessage)) {
        return false;
    }

    document.MarkClean();
    return true;
}

bool ProjectSession::SaveAsCopy(
    std::string *errorMessage) {

    const ProjectManifest *manifest =
        projectManager.GetActiveManifest();

    if (manifest != nullptr &&
        !manifest->runtimeLibrary.empty()) {

        SetError(
            errorMessage,
            "Save As for projects with runtime plugins "
            "requires complete directory copying.");

        return false;
    }

    const std::string sourceName =
        manifest != nullptr
            ? manifest->name
            : "Untitled";

    const std::filesystem::path directory =
        ProjectManager::FindAvailableProjectDirectory(
            defaultProjectsDirectory,
            sourceName + " Copy");

    if (!projectManager.CreateProject(
            directory,
            directory.filename().string(),
            document,
            errorMessage)) {

        return false;
    }

    document.MarkClean();
    return true;
}

bool ProjectSession::CanSwitchProject(
    std::string *errorMessage) const {
    if(assetDatabase.HasActiveImport()){SetError(errorMessage,"Finish or cancel the active asset import before switching projects");return false;}

    if (document.IsDirty() || visualAssetEditor.IsDirty() || tilemapEditor.IsDirty() || tilemapEditor.HasPointerCapture()) {
        SetError(
            errorMessage,
            "Save the current project before "
            "switching projects.");

        return false;
    }

    return true;
}

bool ProjectSession::HasProject() const {
    return projectManager.HasActiveProject();
}

const ProjectManifest *
ProjectSession::GetManifest() const {
    return projectManager.GetActiveManifest();
}

const std::filesystem::path &
ProjectSession::GetProjectDirectory() const {
    return projectManager.GetActiveProjectDirectory();
}

const SceneDocument &
ProjectSession::GetDocument() const {
    return document;
}

SceneDocument &
ProjectSession::GetDocument() {
    return document;
}

ProjectRuntimeHost &
ProjectSession::GetRuntime() {
    return runtimeHost;
}

const ProjectRuntimeHost &
ProjectSession::GetRuntime() const {
    return runtimeHost;
}

assets::AssetDatabase &ProjectSession::GetAssetDatabase() { return assetDatabase; }
const assets::AssetDatabase &ProjectSession::GetAssetDatabase() const { return assetDatabase; }
PrefabLibrary &ProjectSession::GetPrefabLibrary() { return prefabLibrary; }
const PrefabLibrary &ProjectSession::GetPrefabLibrary() const { return prefabLibrary; }

std::span<const SceneObjectTypeDescriptor>
ProjectSession::GetObjectTypes() const {
    const auto runtimeTypes=runtimeHost.GetSceneObjectTypes();
    return runtimeTypes.empty()?std::span<const SceneObjectTypeDescriptor>(authoredObjectTypes):runtimeTypes;
}

std::span<const SceneComponentTypeDescriptor> ProjectSession::GetComponentTypes() const {
    const auto runtimeTypes=runtimeHost.GetSceneComponentTypes();
    combinedComponentTypes=commonComponentTypes;
    combinedComponentTypes.insert(combinedComponentTypes.end(),authoredComponentTypes.begin(),authoredComponentTypes.end());
    for(const auto &type:runtimeTypes) {
        const auto found=std::ranges::find(combinedComponentTypes,type.typeId,&SceneComponentTypeDescriptor::typeId);
        if(found==combinedComponentTypes.end()) combinedComponentTypes.push_back(type); else *found=type;
    }
    return combinedComponentTypes;
}

const SceneObjectTypeDescriptor *
ProjectSession::FindObjectType(
    const SceneObjectTypeId &typeId) const {

    for (const SceneObjectTypeDescriptor &type :
         GetObjectTypes()) {

        if (type.typeId == typeId) {
            return &type;
        }
    }

    return nullptr;
}

const SceneComponentTypeDescriptor *ProjectSession::FindComponentType(const std::string &typeId) const {
    for (const auto &type : GetComponentTypes()) if (type.typeId == typeId) return &type;
    return nullptr;
}

std::optional<SceneObjectId>
ProjectSession::GetSelectedObjectId() const {
    return selectedObjectId;
}

std::span<const SceneObjectId>
ProjectSession::GetSelectedObjectIds() const {
    return selectedObjectIds;
}

bool ProjectSession::IsObjectSelected(const SceneObjectId objectId) const {
    return std::ranges::find(selectedObjectIds, objectId) != selectedObjectIds.end();
}

const SceneObjectData *
ProjectSession::GetSelectedObject() const {
    if (!selectedObjectId.has_value()) {
        return nullptr;
    }

    return document.FindObject(*selectedObjectId);
}

void ProjectSession::SetSelectedObject(
    std::optional<SceneObjectId> objectId) {

    if (objectId.has_value() &&
        document.FindObject(*objectId) == nullptr) {

        objectId.reset();
    }

    selectedObjectId = objectId;
    selectedObjectIds.clear();

    if (objectId.has_value()) {
        selectedObjectIds.push_back(*objectId);
    }

    runtimeHost.SetSelectedObject(
        selectedObjectId);
}

void ProjectSession::SetSelectedObjects(std::vector<SceneObjectId> objectIds) {
    std::unordered_set<SceneObjectId> seen;
    std::erase_if(objectIds, [this, &seen](const SceneObjectId id) {
        return id == 0 || document.FindObject(id) == nullptr || !seen.insert(id).second;
    });

    selectedObjectIds = std::move(objectIds);
    selectedObjectId = selectedObjectIds.empty()
                           ? std::nullopt
                           : std::optional<SceneObjectId>{selectedObjectIds.back()};
    runtimeHost.SetSelectedObject(selectedObjectId);
}

void ProjectSession::AddSelectedObject(const SceneObjectId objectId) {
    if (document.FindObject(objectId) == nullptr || IsObjectSelected(objectId)) {
        return;
    }
    selectedObjectIds.push_back(objectId);
    selectedObjectId = objectId;
    runtimeHost.SetSelectedObject(selectedObjectId);
}

void ProjectSession::ToggleSelectedObject(const SceneObjectId objectId) {
    if (document.FindObject(objectId) == nullptr) {
        return;
    }
    const auto iterator = std::ranges::find(selectedObjectIds, objectId);
    if (iterator == selectedObjectIds.end()) {
        AddSelectedObject(objectId);
        return;
    }
    selectedObjectIds.erase(iterator);
    selectedObjectId = selectedObjectIds.empty()
                           ? std::nullopt
                           : std::optional<SceneObjectId>{selectedObjectIds.back()};
    runtimeHost.SetSelectedObject(selectedObjectId);
}

bool ProjectSession::CreateObject(const std::optional<Vector2f> position) {
    const std::span<
        const SceneObjectTypeDescriptor>
        types = GetObjectTypes();

    if (types.empty()) {
        return false;
    }

    const SceneObjectTypeDescriptor *type =
        &types.front();

    if (const SceneObjectData *selected =
            GetSelectedObject()) {

        if (const SceneObjectTypeDescriptor
                *selectedType =
                    FindObjectType(
                        selected->typeId)) {

            type = selectedType;
        }
    }

    return CreateObjectOfType(type->typeId,position);
}

bool ProjectSession::CreateObjectOfType(const SceneObjectTypeId &typeId,const std::optional<Vector2f> position) {
    const auto *type=FindObjectType(typeId);
    if(!type)return false;
    SceneObjectData object;
    if(runtimeHost.HasRuntime()) {
        object=runtimeHost.CreateDefaultObject(type->typeId);
    } else {
        object.name=type->displayName;object.typeId=type->typeId;
        for(const auto &componentId:type->componentTypeIds) {
            const auto *descriptor=FindComponentType(componentId);
            if(!descriptor)continue;
            SceneComponentData component{descriptor->typeId,descriptor->schemaVersion,{},true,descriptor->editorOnly};
            for(const auto &property:descriptor->properties)
                component.properties.emplace(property.key,property.defaultValue);
            object.components.push_back(std::move(component));
        }
        SynchronizeTransformComponent(object);
    }

    if (position) {
        object.transform.position = *position;
        SynchronizeTransformComponent(object);
    }

    if (object.name.empty()) {
        object.name = type->displayName;
    }

    object.name =
        MakeUniqueObjectName(object.name);

    history.Record(document);

    const SceneObjectId objectId = document.CreateObject(std::move(object));

    SetSelectedObject(objectId);
    DocumentChanged();

    return true;
}

bool ProjectSession::ApplyRuntimeSceneEdits(std::vector<ProjectRuntimeSceneEdit> edits) {
    if (edits.empty()) {
        return false;
    }

    history.Record(document);
    bool changed = false;
    std::optional<SceneObjectId> createdSelection;

    for (ProjectRuntimeSceneEdit &edit : edits) {
        if(edit.kind==ProjectRuntimeSceneEditKind::RemoveObject){changed|=document.RemoveObject(edit.object.id);continue;}
        if(edit.kind==ProjectRuntimeSceneEditKind::ReplaceObject){changed|=document.ReplaceObjectData(edit.object.id,std::move(edit.object));continue;}
        if(edit.kind==ProjectRuntimeSceneEditKind::SetComponentEnabled){
            if(const auto *existing=document.FindObject(edit.object.id)){
                SceneObjectData replacement=*existing;
                const auto component=std::ranges::find(replacement.components,edit.componentTypeId,&SceneComponentData::typeId);
                if(component!=replacement.components.end()){component->enabled=edit.componentEnabled;changed|=document.ReplaceObjectData(replacement.id,std::move(replacement));}
            }
            continue;
        }
        if (edit.kind == ProjectRuntimeSceneEditKind::ReplaceObjectType ||
            edit.kind == ProjectRuntimeSceneEditKind::RemoveObjectType) {
            std::vector<SceneObjectId> removals;
            for (const SceneObjectData &existing : document.GetObjects()) {
                if (existing.typeId == edit.targetTypeId) {
                    removals.push_back(existing.id);
                }
            }
            for (const SceneObjectId id : removals) {
                changed |= document.RemoveObject(id);
            }
        }

        if (edit.kind == ProjectRuntimeSceneEditKind::RemoveObjectType) {
            continue;
        }

        SceneObjectData object = std::move(edit.object);
        if (object.typeId.empty()) {
            continue;
        }
        if (object.name.empty()) {
            if (const SceneObjectTypeDescriptor *type = FindObjectType(object.typeId); type != nullptr) {
                object.name = type->displayName;
            }
        }
        object.name = MakeUniqueObjectName(object.name);
        const SceneObjectId id = document.CreateObject(std::move(object));
        if (id != 0) {
            changed = true;
            createdSelection = id;
        }
    }

    if (!changed) {
        history.Undo(document);
        return false;
    }

    SetSelectedObject(createdSelection);
    DocumentChanged();
    return true;
}

bool ProjectSession::DeleteSelectedObject() {
    if (selectedObjectIds.empty()) {
        return false;
    }

    history.Record(document);

    bool removed = false;
    for (const SceneObjectId objectId : selectedObjectIds) {
        removed |= document.RemoveObject(objectId);
    }

    if (!removed) {

        history.Undo(document);
        return false;
    }

    SetSelectedObject(std::nullopt);
    DocumentChanged();

    return true;
}

bool ProjectSession::RenameSelectedObject(std::string name) {
    if (!selectedObjectId || name.empty()) return false;
    history.Record(document);
    if (!document.RenameObject(*selectedObjectId, std::move(name))) { history.Undo(document); return false; }
    DocumentChanged(); return true;
}

bool ProjectSession::ReparentSelectedObjects(const SceneObjectId parentId, const std::int32_t siblingOrder) {
    if (selectedObjectIds.empty()) return false;
    history.Record(document);
    bool changed = false;
    std::int32_t order = siblingOrder;
    for (const auto id : selectedObjectIds) {
        if (id == parentId) continue;
        changed |= document.SetParent(id, parentId, order);
        if (order >= 0) ++order;
    }
    if (!changed) { history.Undo(document); return false; }
    DocumentChanged(); return true;
}

bool ProjectSession::DuplicateSelectedObjects() {
    if (selectedObjectIds.empty()) return false;
    history.Record(document);
    std::vector<SceneObjectId> copies;
    for (const auto id : selectedObjectIds) {
        if (const auto copy = document.DuplicateObject(id, true); copy != 0) copies.push_back(copy);
    }
    if (copies.empty()) { history.Undo(document); return false; }
    SetSelectedObjects(std::move(copies)); DocumentChanged(); return true;
}

bool ProjectSession::GroupSelectedObjects(std::string groupName) {
    if (selectedObjectIds.empty()) return false;
    history.Record(document);
    SceneObjectData group;
    group.name = MakeUniqueObjectName(groupName.empty() ? "GROUP" : groupName);
    group.typeId = "pipeframe.group";
    const auto groupId = document.CreateObject(std::move(group));
    bool changed = groupId != 0;
    for (const auto id : selectedObjectIds) changed |= document.SetParent(id, groupId);
    if (!changed) { history.Undo(document); return false; }
    SetSelectedObject(groupId); DocumentChanged(); return true;
}

bool ProjectSession::SetSelectedObjectsVisible(const bool visible) {
    if (selectedObjectIds.empty()) return false;
    history.Record(document); bool changed = false;
    for (const auto id : selectedObjectIds) changed |= document.SetVisible(id, visible);
    if (!changed) { history.Undo(document); return false; }
    DocumentChanged(); return true;
}

bool ProjectSession::SetSelectedObjectsLocked(const bool locked) {
    if (selectedObjectIds.empty()) return false;
    history.Record(document); bool changed = false;
    for (const auto id : selectedObjectIds) changed |= document.SetLocked(id, locked);
    if (!changed) { history.Undo(document); return false; }
    DocumentChanged(); return true;
}

bool ProjectSession::SetSelectedObjectsLayer(std::string layer) {
    if (selectedObjectIds.empty() || layer.empty()) return false;
    history.Record(document); bool changed = false;
    for (const auto id : selectedObjectIds) changed |= document.SetLayer(id, layer);
    if (!changed) { history.Undo(document); return false; }
    DocumentChanged(); return true;
}

bool ProjectSession::SetSelectedObjectsTags(std::vector<std::string> tags) {
    if (selectedObjectIds.empty()) return false;
    history.Record(document); bool changed = false;
    for (const auto id : selectedObjectIds) changed |= document.SetTags(id, tags);
    if (!changed) { history.Undo(document); return false; }
    DocumentChanged(); return true;
}

bool ProjectSession::AddSelectedComponent(SceneComponentData component) {
    if (!selectedObjectId || component.typeId.empty()) return false;
    history.Record(document);
    if (!document.AddComponent(*selectedObjectId, std::move(component))) { history.Undo(document); return false; }
    DocumentChanged(); return true;
}

bool ProjectSession::RemoveSelectedComponent(const std::string &componentTypeId) {
    if (!selectedObjectId) return false;
    if (const auto *type=FindComponentType(componentTypeId); type && !type->removable) return false;
    history.Record(document);
    if (!document.RemoveComponent(*selectedObjectId, componentTypeId)) { history.Undo(document); return false; }
    DocumentChanged(); return true;
}

bool ProjectSession::SetSelectedComponentProperty(const std::string &componentTypeId,
                                                  std::string key, PropertyValue value) {
    if (selectedObjectIds.empty() || componentTypeId.empty() || key.empty()) return false;
    const SceneComponentTypeDescriptor *componentType = FindComponentType(componentTypeId);
    const PropertyDescriptor *property = componentType
                                             ? FindPropertyDescriptor(componentType->properties, key)
                                             : nullptr;
    if (componentType && (!property || !property->editable || property->editorHint == "telemetry" ||
                          !ValidatePropertyValue(*property, value))) return false;
    if (const auto *reference = std::get_if<SceneObjectReference>(&value);
        reference && reference->objectId != 0 && !document.FindObject(reference->objectId)) return false;

    if(const auto *reference=std::get_if<AssetReference>(&value);reference && !reference->assetId.empty() &&
       property && property->editorHint.starts_with("asset:")) {
        const auto *asset=assetDatabase.Find(reference->assetId);
        if(!asset || asset->state!=assets::AssetState::Ready ||
           property->editorHint.substr(6)!=assets::ToString(asset->type))return false;
    }

    const SceneObjectData *primary = GetSelectedObject();
    const SceneComponentData *primaryComponent = primary ? FindComponent(*primary, componentTypeId) : nullptr;
    const PropertyValue *primaryValue = nullptr;
    if (primaryComponent) {
        const auto found = primaryComponent->properties.find(key);
        if (found != primaryComponent->properties.end()) primaryValue = &found->second;
    }

    SceneDocument candidate=document;
    bool changed = false;
    std::vector<ProjectRuntimeComponentEdit> runtimeEdits;
    for (const auto objectId : selectedObjectIds) {
        PropertyValue applied = value;
        if (const SceneObjectData *object = document.FindObject(objectId)) {
            if (const SceneComponentData *component = FindComponent(*object, componentTypeId)) {
                if (const auto found = component->properties.find(key); found != component->properties.end()) {
                    if (const auto *next = std::get_if<Vector2f>(&value)) {
                        if (const auto *current = std::get_if<Vector2f>(&found->second)) {
                            Vector2f merged = *current;
                            const auto *base = primaryValue ? std::get_if<Vector2f>(primaryValue) : nullptr;
                            if (!base || next->x != base->x) merged.x = next->x;
                            if (!base || next->y != base->y) merged.y = next->y;
                            applied = merged;
                        }
                    } else if (const auto *next = std::get_if<Color>(&value)) {
                        if (const auto *current = std::get_if<Color>(&found->second)) {
                            Color merged = *current;
                            const auto *base = primaryValue ? std::get_if<Color>(primaryValue) : nullptr;
                            if (!base || next->red != base->red) merged.red = next->red;
                            if (!base || next->green != base->green) merged.green = next->green;
                            if (!base || next->blue != base->blue) merged.blue = next->blue;
                            if (!base || next->alpha != base->alpha) merged.alpha = next->alpha;
                            applied = merged;
                        }
                    }
                }
            }
        }
        if (candidate.SetComponentProperty(objectId, componentTypeId, key, applied)) {
            changed = true;
            runtimeEdits.push_back({objectId, componentTypeId, key, std::move(applied)});
        }
    }
    if (!changed) {
        return false;
    }
    std::string validationError;
    if (!runtimeHost.ValidateComponentEdits(runtimeEdits,validationError)) return false;
    history.Record(document);
    document=std::move(candidate);
    if (!runtimeHost.ApplyComponentEdits(runtimeEdits)) DocumentChanged();
    else runtimeHost.SetSelectedObject(selectedObjectId);
    return true;
}

bool ProjectSession::ResetSelectedComponentProperty(const std::string &componentTypeId,
                                                    const std::string &key) {
    const SceneComponentTypeDescriptor *component = FindComponentType(componentTypeId);
    if (!component) return false;
    const PropertyDescriptor *property = FindPropertyDescriptor(component->properties, key);
    return property && property->editable && SetSelectedComponentProperty(componentTypeId, key,
                                                                           property->defaultValue);
}

std::optional<SceneComponentData> ProjectSession::CopySelectedComponent(
    const std::string &componentTypeId) const {
    const SceneObjectData *object = GetSelectedObject();
    if (!object) return std::nullopt;
    if (const SceneComponentData *component = FindComponent(*object, componentTypeId)) return *component;
    return std::nullopt;
}

bool ProjectSession::PasteComponentToSelected(const SceneComponentData &component) {
    if (selectedObjectIds.empty() || component.typeId.empty() ||
        component.typeId == Transform2DComponentTypeId) return false;
    const SceneComponentTypeDescriptor *type = FindComponentType(component.typeId);
    if (type) {
        for (const auto &[key, value] : component.properties) {
            if (const PropertyDescriptor *property = FindPropertyDescriptor(type->properties, key);
                property && property->editable && !ValidatePropertyValue(*property, value)) return false;
        }
    }
    history.Record(document);
    bool changed = false;
    for (const SceneObjectId objectId : selectedObjectIds) {
        const SceneObjectData *object = document.FindObject(objectId);
        if (!object) continue;
        if (!FindComponent(*object, component.typeId)) {
            changed |= document.AddComponent(objectId, component);
            continue;
        }
        for (const auto &[key, value] : component.properties) {
            const PropertyDescriptor *property = type ? FindPropertyDescriptor(type->properties, key) : nullptr;
            if (property && !property->editable) continue;
            changed |= document.SetComponentProperty(objectId, component.typeId, key, value);
        }
    }
    if (!changed) { history.Undo(document); return false; }
    DocumentChanged();
    return true;
}

bool ProjectSession::SetSceneSettings(SceneSettings settings) {
    history.Record(document);
    if (!document.SetSettings(std::move(settings))) { history.Undo(document); return false; }
    DocumentChanged(); return true;
}

bool ProjectSession::AddSceneConnection(SceneConnectionData connection) {
    history.Record(document);
    if (!document.AddConnection(std::move(connection))) { history.Undo(document); return false; }
    DocumentChanged(); return true;
}

bool ProjectSession::RemoveSceneConnection(const std::uint64_t connectionId) {
    history.Record(document);
    if (!document.RemoveConnection(connectionId)) { history.Undo(document); return false; }
    DocumentChanged(); return true;
}

bool ProjectSession::CreatePrefabFromSelected(const std::string &prefabId,
                                              std::string *errorMessage) {
    if (!HasProject() || !selectedObjectId || !IsSafePrefabId(prefabId)) {
        SetError(errorMessage, "A project, selected root, and safe prefab ID are required.");
        return false;
    }
    auto prefab = prefabLibrary.Create(prefabId, document, *selectedObjectId, errorMessage);
    const auto path = PrefabPath(GetProjectDirectory(), prefabId);
    if (!prefab || !prefabLibrary.Save(*prefab, path, errorMessage))
        return false;
    const auto relative = path.lexically_relative(GetProjectDirectory());
    const auto existing = std::ranges::find_if(assetDatabase.GetAssets(), [&](const auto &asset) {
        return asset.sourcePath == relative;
    });
    if (existing == assetDatabase.GetAssets().end()) {
        if (!assetDatabase.ImportNow({path, "Prefabs", assets::AssetType::Prefab}, errorMessage)) return false;
    } else if (!assetDatabase.Reimport(existing->id, errorMessage)) {
        return false;
    }
    history.Record(document);
    if (!prefabLibrary.AdoptInstance(*prefab, document, *selectedObjectId, errorMessage)) {
        history.Undo(document);
        return false;
    }
    DocumentChanged();
    return true;
}

bool ProjectSession::InstantiatePrefab(const std::string &prefabId, const SceneObjectId parentId,
                                       const SceneTransform placement, std::string *errorMessage) {
    if (!HasProject() || !IsSafePrefabId(prefabId)) {
        SetError(errorMessage, "A project and safe prefab ID are required.");
        return false;
    }
    auto prefab = prefabLibrary.Load(PrefabPath(GetProjectDirectory(), prefabId), errorMessage);
    if (!prefab) return false;
    history.Record(document);
    auto instance = prefabLibrary.Instantiate(*prefab, document, parentId, placement, errorMessage);
    if (!instance) {
        history.Undo(document);
        return false;
    }
    SetSelectedObject(instance->rootObjectId);
    DocumentChanged();
    return true;
}

bool ProjectSession::ApplySelectedPrefab(std::string *errorMessage) {
    const auto *selected = GetSelectedObject();
    if (!selected || selected->prefabLinks.empty()) {
        SetError(errorMessage, "Select an object in a prefab instance.");
        return false;
    }
    const auto link = selected->prefabLinks.back();
    auto prefab = prefabLibrary.Load(PrefabPath(GetProjectDirectory(), link.prefabId), errorMessage);
    if (!prefab) return false;
    history.Record(document);
    if (!prefabLibrary.ApplyInstance(*prefab, document, link.instanceRootId, errorMessage) ||
        !prefabLibrary.RevertInstance(*prefab, document, link.instanceRootId, errorMessage) ||
        !prefabLibrary.Save(*prefab, PrefabPath(GetProjectDirectory(), link.prefabId), errorMessage)) {
        history.Undo(document);
        return false;
    }
    const auto relative = PrefabPath(GetProjectDirectory(), link.prefabId).lexically_relative(GetProjectDirectory());
    const auto asset = std::ranges::find_if(assetDatabase.GetAssets(), [&](const auto &item) {
        return item.sourcePath == relative;
    });
    if (asset != assetDatabase.GetAssets().end()) assetDatabase.Reimport(asset->id);
    DocumentChanged();
    return true;
}

bool ProjectSession::RevertSelectedPrefab(std::string *errorMessage) {
    const auto *selected = GetSelectedObject();
    if (!selected || selected->prefabLinks.empty()) {
        SetError(errorMessage, "Select an object in a prefab instance.");
        return false;
    }
    const auto link = selected->prefabLinks.back();
    auto prefab = prefabLibrary.Load(PrefabPath(GetProjectDirectory(), link.prefabId), errorMessage);
    if (!prefab) return false;
    history.Record(document);
    if (!prefabLibrary.RevertInstance(*prefab, document, link.instanceRootId, errorMessage)) {
        history.Undo(document);
        return false;
    }
    SetSelectedObject(link.instanceRootId);
    DocumentChanged();
    return true;
}

bool ProjectSession::UnpackSelectedPrefab(std::string *errorMessage) {
    const auto *selected = GetSelectedObject();
    if (!selected || selected->prefabLinks.empty()) {
        SetError(errorMessage, "Select an object in a prefab instance.");
        return false;
    }
    const auto rootId = selected->prefabLinks.back().instanceRootId;
    history.Record(document);
    if (!prefabLibrary.UnpackInstance(document, rootId, errorMessage)) {
        history.Undo(document);
        return false;
    }
    SetSelectedObject(rootId);
    DocumentChanged();
    return true;
}

bool ProjectSession::SetSelectedTransform(
    const SceneTransform &transform) {

    if (selectedObjectIds.empty()) {
        return false;
    }

    history.Record(document);

    if (!document.SetTransform(
            *selectedObjectId,
            transform)) {

        history.Undo(document);
        return false;
    }

    DocumentChanged();
    return true;
}

bool ProjectSession::SetSelectedProperty(
    const std::string &key,
    const PropertyValue &value) {

    if (!selectedObjectId.has_value()) {
        return false;
    }

    const SceneObjectData *primary = GetSelectedObject();
    const SceneObjectTypeDescriptor *type = primary ? FindObjectType(primary->typeId) : nullptr;
    const PropertyDescriptor *property = type ? FindPropertyDescriptor(type->properties, key) : nullptr;
    if (type && (!property || !property->editable || property->editorHint == "telemetry" ||
                 !ValidatePropertyValue(*property, value))) return false;
    if (const auto *reference = std::get_if<SceneObjectReference>(&value);
        reference && reference->objectId != 0 && !document.FindObject(reference->objectId)) return false;

    history.Record(document);

    bool changed = false;
    for (const auto objectId : selectedObjectIds) changed |= document.SetProperty(objectId, key, value);
    if (!changed) {

        history.Undo(document);
        return false;
    }

    DocumentChanged();
    return true;
}

bool ProjectSession::ResetSelectedProperty(const std::string &key) {
    const SceneObjectData *object = GetSelectedObject();
    const SceneObjectTypeDescriptor *type = object ? FindObjectType(object->typeId) : nullptr;
    const PropertyDescriptor *property = type ? FindPropertyDescriptor(type->properties, key) : nullptr;
    return property && property->editable && SetSelectedProperty(key, property->defaultValue);
}

bool ProjectSession::AssignAssetToSelectedProperty(const std::string &componentTypeId,
                                                   const std::string &key,
                                                   const assets::AssetId &assetId) {
    const assets::AssetRecord *asset = assetDatabase.Find(assetId);
    if (!asset || asset->state != assets::AssetState::Ready) return false;
    const PropertyDescriptor *property=nullptr;
    if(componentTypeId.empty()){
        const auto *object=GetSelectedObject();const auto *type=object?FindObjectType(object->typeId):nullptr;
        if(type)property=FindPropertyDescriptor(type->properties,key);
    }else if(const auto *type=FindComponentType(componentTypeId))property=FindPropertyDescriptor(type->properties,key);
    if(!property||!property->editable||property->kind!=PropertyKind::AssetReference)return false;
    if(property->editorHint.starts_with("asset:")&&property->editorHint.substr(6)!=assets::ToString(asset->type))return false;
    if (componentTypeId.empty()) return SetSelectedProperty(key, AssetReference{assetId});
    return SetSelectedComponentProperty(componentTypeId, key, AssetReference{assetId});
}

bool ProjectSession::AssignAssetToFirstSelectedAssetProperty(const assets::AssetId &assetId) {
    const SceneObjectData *object = GetSelectedObject();
    if (!object) return false;
    const auto *asset=assetDatabase.Find(assetId);if(!asset||asset->state!=assets::AssetState::Ready)return false;
    std::vector<std::pair<std::string,std::string>> targets;
    const auto append=[&](const auto &type,const std::string &component){
        for(const auto &property:type.properties)
            if(property.kind==PropertyKind::AssetReference&&property.editable&&
               (!property.editorHint.starts_with("asset:")||property.editorHint.substr(6)==assets::ToString(asset->type)))
                targets.emplace_back(component,property.key);
    };
    for(const auto &component:object->components)if(const auto *type=FindComponentType(component.typeId))append(*type,component.typeId);
    if(const auto *type=FindObjectType(object->typeId))append(*type,{});
    // Viewport drop is unambiguous only with one compatible target; the browser
    // offers explicit property buttons when the object has several.
    return targets.size()==1&&AssignAssetToSelectedProperty(targets.front().first,targets.front().second,assetId);
}

std::vector<assets::AssetId> ProjectSession::FindMissingAssetReferences() const {
    std::vector<assets::AssetId> result;
    const auto inspect = [&](const PropertyMap &properties) {
        for (const auto &[key, value] : properties) {
            (void)key;
            if (const auto *reference = std::get_if<AssetReference>(&value);
                reference && !reference->assetId.empty()) {
                const assets::AssetRecord *asset = assetDatabase.Find(reference->assetId);
                if ((!asset || asset->state != assets::AssetState::Ready) &&
                    std::ranges::find(result, reference->assetId) == result.end())
                    result.push_back(reference->assetId);
            }
        }
    };
    for (const auto &object : document.GetObjects()) {
        inspect(object.properties);
        for (const auto &component : object.components) inspect(component.properties);
    }
    std::ranges::sort(result);
    return result;
}

bool ProjectSession::RepairAssetReferences(const assets::AssetId &missingAssetId,
                                           const assets::AssetId &replacementAssetId) {
    const assets::AssetRecord *replacement = assetDatabase.Find(replacementAssetId);
    if (missingAssetId.empty() || !replacement || replacement->state != assets::AssetState::Ready) return false;
    history.Record(document);
    bool changed = false;
    for (const auto &object : document.GetObjects()) {
        for (const auto &[key, value] : object.properties)
            if (const auto *reference = std::get_if<AssetReference>(&value);
                reference && reference->assetId == missingAssetId)
                changed |= document.SetProperty(object.id, key, AssetReference{replacementAssetId});
        for (const auto &component : object.components)
            for (const auto &[key, value] : component.properties)
                if (const auto *reference = std::get_if<AssetReference>(&value);
                    reference && reference->assetId == missingAssetId)
                    changed |= document.SetComponentProperty(object.id, component.typeId, key,
                                                             AssetReference{replacementAssetId});
    }
    if (!changed) { history.Undo(document); return false; }
    DocumentChanged(); return true;
}

bool ProjectSession::BeginContinuousEdit() {
    if (selectedObjectIds.empty() ||
        history.HasContinuousEdit()) {

        return false;
    }

    history.BeginContinuousEdit(document);
    return true;
}

bool ProjectSession::UpdateSelectedTransform(
    const SceneTransform &transform) {

    if (!selectedObjectId.has_value() ||
        !history.HasContinuousEdit()) {

        return false;
    }

    if (!document.SetTransform(
            *selectedObjectId,
            transform)) {

        return false;
    }

    SynchronizeRuntime();
    return true;
}

bool ProjectSession::UpdateSelectedTransforms(
    const std::vector<std::pair<SceneObjectId, SceneTransform>> &transforms) {
    if (!history.HasContinuousEdit() || transforms.empty()) {
        return false;
    }

    bool changed = false;
    for (const auto &[objectId, transform] : transforms) {
        if (IsObjectSelected(objectId)) {
            changed |= document.SetTransform(objectId, transform);
        }
    }

    if (changed) {
        DocumentChanged();
    }
    return changed;
}

bool ProjectSession::CommitContinuousEdit() {
    if (!history.HasContinuousEdit()) {
        return false;
    }

    history.CommitContinuousEdit();
    DocumentChanged();

    return true;
}

bool ProjectSession::CancelContinuousEdit() {
    if (!history.CancelContinuousEdit(document)) {
        return false;
    }

    ValidateSelection();
    DocumentChanged();

    return true;
}

bool ProjectSession::Undo() {
    if(tilemapEditor.IsEnabled())return tilemapEditor.Undo();
    if (!history.Undo(document)) {
        return false;
    }

    ValidateSelection();
    DocumentChanged();

    return true;
}

bool ProjectSession::Redo() {
    if(tilemapEditor.IsEnabled())return tilemapEditor.Redo();
    if (!history.Redo(document)) {
        return false;
    }

    ValidateSelection();
    DocumentChanged();

    return true;
}

bool ProjectSession::CanUndo() const {
    return tilemapEditor.IsEnabled()?tilemapEditor.CanUndo():history.CanUndo();
}

bool ProjectSession::CanRedo() const {
    return tilemapEditor.IsEnabled()?tilemapEditor.CanRedo():history.CanRedo();
}

void ProjectSession::SynchronizeRuntime() {
    runtimeHost.SynchronizeScene(
        document.GetObjects());

    runtimeHost.SetSelectedObject(
        selectedObjectId);
}

bool ProjectSession::OpenProject(
    const std::filesystem::path &path,
    std::string *errorMessage) {

    if(!CanSwitchProject(errorMessage))return false;
    SceneDocument loadedDocument;

    if (!projectManager.OpenProject(
            path,
            loadedDocument,
            errorMessage)) {

        return false;
    }

    ActivateDocument(std::move(loadedDocument));
    return true;
}

void ProjectSession::ActivateDocument(
    SceneDocument newDocument) {

    tilemapEditor.ClearBrushes();
    runtimeHost.Unload();

    document = std::move(newDocument);
    document.MarkClean();

    selectedObjectId.reset();
    selectedObjectIds.clear();
    history.Clear();

    visualAssetEditor.Close();editingVisuals.Unload();
    tilemapEditor.Close(true);
    std::string assetError;
    if (!assetDatabase.Open(projectManager.GetActiveProjectDirectory(), &assetError))
        std::cerr << "Project opened, but its asset database could not be loaded: " << assetError << '\n';

    LoadAuthoredObjectTypes();
    LoadActiveRuntime();
    MigrateDocumentToRuntimeComponents();
    SynchronizeRuntime();
}

void ProjectSession::LoadAuthoredObjectTypes(){
    authoredObjectTypes.clear();
    authoredComponentTypes.clear();
    const auto componentDirectory=projectManager.GetActiveProjectDirectory()/"Config"/"Components";
    std::error_code componentError;
    if(std::filesystem::is_directory(componentDirectory,componentError))
        for(const auto &entry:std::filesystem::directory_iterator(componentDirectory,componentError)){
            if(componentError||!entry.is_regular_file()||entry.path().extension()!=".pfcomponent")continue;
            std::ifstream input(entry.path());std::string header,id,name;unsigned version{};
            if(!(input>>header>>version>>id>>name)||header!="PIPEFRAME_COMPONENT"||version!=1||id.empty()||name.empty())continue;
            authoredComponentTypes.push_back({id,name,1,true,false,{}});
        }
    const auto directory=projectManager.GetActiveProjectDirectory()/"Assets"/"Prefabs";
    std::error_code error;
    if(!std::filesystem::is_directory(directory,error))return;
    for(const auto &entry:std::filesystem::directory_iterator(directory,error)){
        if(error||!entry.is_regular_file()||entry.path().extension()!=".pftype")continue;
        std::ifstream input(entry.path());std::string header,name;unsigned version{};
        if(!(input>>header>>version>>name)||header!="PIPEFRAME_OBJECT_TYPE"||version!=1||name.empty())continue;
        SceneObjectTypeDescriptor type;
        type.typeId="project."+entry.path().stem().string();
        type.displayName=name;
        std::string component;
        while(input>>component)type.componentTypeIds.push_back(component);
        if(type.componentTypeIds.empty())type.componentTypeIds.push_back(Transform2DComponentTypeId);
        authoredObjectTypes.push_back(std::move(type));
    }
    std::ranges::sort(authoredObjectTypes,{},&SceneObjectTypeDescriptor::displayName);
}

void ProjectSession::LoadActiveRuntime() {
    const ProjectManifest *manifest =
        projectManager.GetActiveManifest();

    if (manifest == nullptr ||
        manifest->runtimeLibrary.empty()) {

        return;
    }

    std::string runtimeError;

    if (!runtimeHost.Load(
            projectManager
                .GetActiveProjectDirectory(),
            manifest->runtimeLibrary,
            &runtimeError)) {

        std::cerr
            << "Project opened, but its runtime "
               "could not be loaded: "
            << runtimeError << '\n';
    }
}

void ProjectSession::MigrateDocumentToRuntimeComponents(){
    if(!runtimeHost.HasRuntime())return;
    const auto objectTypes=runtimeHost.GetSceneObjectTypes();
    const auto componentTypes=runtimeHost.GetSceneComponentTypes();
    const std::vector<SceneObjectData> objects(document.GetObjects().begin(),document.GetObjects().end());
    for(auto object:objects){
        const auto type=std::ranges::find(objectTypes,object.typeId,&SceneObjectTypeDescriptor::typeId);
        if(type==objectTypes.end())continue;
        bool changed=false;
        for(const auto &componentTypeId:type->componentTypeIds){
            if(std::ranges::find(object.components,componentTypeId,&SceneComponentData::typeId)!=object.components.end())continue;
            const auto descriptor=std::ranges::find(componentTypes,componentTypeId,&SceneComponentTypeDescriptor::typeId);
            if(descriptor==componentTypes.end())continue;
            SceneComponentData component{descriptor->typeId,descriptor->schemaVersion,{},true,descriptor->editorOnly};
            for(const auto &property:descriptor->properties){
                const auto legacy=object.properties.find(property.key);
                component.properties.emplace(property.key,legacy==object.properties.end()?property.defaultValue:legacy->second);
                if(legacy!=object.properties.end())object.properties.erase(legacy);
            }
            object.components.push_back(std::move(component));changed=true;
        }
        for(auto &component:object.components)for(auto &[key,value]:component.properties)
            if(auto *reference=std::get_if<AssetReference>(&value);reference&&reference->assetId.starts_with("source:"))
                if(const auto *asset=assetDatabase.Find(reference->assetId)){reference->assetId=asset->id;changed=true;}
        if(changed)document.ReplaceObjectData(object.id,std::move(object));
    }
}

void ProjectSession::ValidateSelection() {
    std::erase_if(selectedObjectIds, [this](const SceneObjectId id) {
        return document.FindObject(id) == nullptr;
    });

    if (!selectedObjectId.has_value() ||
        document.FindObject(*selectedObjectId) == nullptr ||
        !IsObjectSelected(*selectedObjectId)) {
        selectedObjectId = selectedObjectIds.empty()
                               ? std::nullopt
                               : std::optional<SceneObjectId>{selectedObjectIds.back()};
    }

    runtimeHost.SetSelectedObject(
        selectedObjectId);
}

void ProjectSession::DocumentChanged() {
    SynchronizeRuntime();
}

std::string ProjectSession::MakeUniqueObjectName(
    const std::string &baseName) const {

    const std::string safeBase =
        baseName.empty()
            ? "OBJECT"
            : baseName;

    const auto nameExists =
        [this](const std::string &name) {
            for (const SceneObjectData &object :
                 document.GetObjects()) {

                if (object.name == name) {
                    return true;
                }
            }

            return false;
        };

    if (!nameExists(safeBase)) {
        return safeBase;
    }

    for (std::size_t suffix = 2;; ++suffix) {
        const std::string candidate =
            safeBase + " " +
            std::to_string(suffix);

        if (!nameExists(candidate)) {
            return candidate;
        }
    }
}

void ProjectSession::SetError(
    std::string *errorMessage,
    std::string message) {

    if (errorMessage != nullptr) {
        *errorMessage = std::move(message);
    }
}

} // namespace pipeframe::editor

namespace pipeframe::editor {
bool ProjectSession::LoadBuiltRuntime(const std::filesystem::path &library,std::string *error){
    if(!HasProject()){if(error)*error="No active project";return false;}
    try {
        tilemapEditor.ClearBrushes();
        const bool loaded=runtimeHost.HasRuntime()
            ?runtimeHost.ReloadFrom(GetProjectDirectory()/library,GetDocument().GetObjects(),GetSelectedObjectId(),error)
            :runtimeHost.Load(GetProjectDirectory(),library,error);
        tilemapEditor.SetBrushes(runtimeHost.EnvironmentBrushes());
        if(!loaded)return false;
        runtimeHost.SynchronizeScene(GetDocument().GetObjects());runtimeHost.SetSelectedObject(GetSelectedObjectId());
        return projectManager.SetRuntimeLibrary(library,error);
    }catch(const std::exception &exception){if(error)*error=exception.what();return false;}
}
}
