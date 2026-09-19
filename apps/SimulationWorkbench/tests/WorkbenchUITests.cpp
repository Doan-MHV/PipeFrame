#if defined(__unix__) || defined(__APPLE__)
#include <sys/resource.h>
#endif
#include <PipeFrame/Backend/SFML/InputEventAdapter.h>
#include <PipeFrame/Backend/SFML/RenderContextAdapter.h>
#include <PipeFrame/Backend/SFML/DockSplitter.h>
#include <PipeFrame/Resources/ImageData.h>
#include <PipeFrame/Environment/TilemapSerializer.h>
#include <PipeFrame/Environment/PlaygroundGeometry.h>
#include <PipeFrame/Components/TilemapComponent.h>
#include <chrono>
#include "../Backend/SFML/WorkbenchView.h"
#include "../Backend/SFML/WorkbenchLayout.h"
#include "../Editor/WorkbenchInput.h"
#include "../Editor/HierarchyPanel.h"
#include "../Editor/InspectorPanel.h"
#include "../Editor/AssetBrowserPanel.h"
#include "../Editor/WorkspaceToolsPanel.h"
#include "../Editor/ViewportToolbar.h"
#include "../Editor/ProjectSession.h"
#include "../Editor/ProjectScaffolder.h"
#include <PipeFrame/Backend/SFML/UI/TextField.h>
#include <PipeFrame/Backend/SFML/UI/ScrollPanel.h>
#include "../Runtime/SimulationSession.h"
#include <PipeFrame/Render/RenderContext.h>
#include <PipeFrame/Backend/SFML/UI/Label.h>
#include <PipeFrame/Backend/SFML/UI/TextButton.h>
#include <PipeFrame/Backend/SFML/UI/SimulationTransport.h>
#include <PipeFrame/Backend/SFML/UI/PopupLayer.h>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Image.hpp>
#include <iostream>
#include <fstream>
#include <cmath>
#include <cctype>
using namespace pipeframe;
using namespace pipeframe::editor;
namespace {
struct BrushPreviewSurface final : RenderSurface {
    std::vector<Vertex2D> footprint,ring;
    Canvas GetCanvas() override {return {this,[](void *self,std::span<const Vertex2D> vertices,PrimitiveTopology,const RenderState&){
        auto &out=static_cast<BrushPreviewSurface*>(self)->footprint;
        for(const auto &vertex:vertices){if(vertex.color==Color{255,220,60,55})out.push_back(vertex);
            if(vertex.color==Color{255,220,60,255})static_cast<BrushPreviewSurface*>(self)->ring.push_back(vertex);
        }
    }};}
    Vector2u GetSize() const override{return {100,100};}
    void SetScreenSize(Vector2u) override{} void BeginWorld(const Camera2D&) override{} void BeginScreen() override{}
    Rectanglei Viewport(const Camera2D&) const override{return {{},{100,100}};}
    Vector2f PixelToWorld(Vector2i p,const Camera2D&) const override{return {float(p.x),float(p.y)};}
    Vector2i WorldToPixel(Vector2f p,const Camera2D&) const override{return {int(p.x),int(p.y)};}
};
void DispatchInput(WorkbenchInput &input,const sf::Event &event,RenderContext &context) {
    if(const auto neutral=pipeframe::backend::sfml::FromBackend(event))input.HandleEvent(*neutral,context);
}

bool Check(bool value,const char *message) { if(!value) std::cerr<<"FAILED: "<<message<<'\n'; return value; }
pipeframe::backend::sfml::DockSplitter *FindSplitter(Widget &root) {
    if (auto *splitter = dynamic_cast<pipeframe::backend::sfml::DockSplitter *>(&root)) return splitter;
    for (std::size_t i=0; i<root.GetChildCount(); ++i)
        if (auto *splitter = FindSplitter(*root.GetChild(i))) return splitter;
    return nullptr;
}
ScrollPanel *FindScroll(Widget &root) {
    if(auto *scroll=dynamic_cast<ScrollPanel*>(&root))return scroll;
    for(std::size_t i=0;i<root.GetChildCount();++i)if(auto *scroll=FindScroll(*root.GetChild(i)))return scroll;
    return nullptr;
}
TextField *FindInput(Widget &root) {
    if(auto *field=dynamic_cast<TextField *>(&root))return field;
    for(std::size_t i=0;i<root.GetChildCount();++i)if(auto *field=FindInput(*root.GetChild(i)))return field;
    return nullptr;
}
TextButton *FindActionInTree(Widget &root,const std::string &text) {
    if(auto *button=dynamic_cast<TextButton *>(&root)) {
        for(std::size_t i=0;i<button->GetChildCount();++i)
            if(auto *label=dynamic_cast<Label *>(button->GetChild(i));label && label->GetText()==text) return button;
    }
    for(std::size_t i=0;i<root.GetChildCount();++i) if(auto *button=FindActionInTree(*root.GetChild(i),text)) return button;
    return nullptr;
}
TextButton *FindAction(Widget &root,const std::string &text) {
    root.Update(0); // Mounted UI commits model changes at the frame boundary.
    return FindActionInTree(root,text);
}
}
int main(int argc,char **argv) {
    sf::RenderWindow window(sf::VideoMode({1100,800}),"Workbench UI verification"); window.setVisible(false);
    auto context = pipeframe::backend::sfml::MakeRenderContext(window);
    WorkbenchView view;
    if(!view.Load(PIPEFRAME_TEST_FONT)) return 1;
    const auto workspaceLayoutPath=std::filesystem::temp_directory_path()/"pipeframe-workbench-18f.layout";
    std::filesystem::remove(workspaceLayoutPath);view.SetWorkspaceLayoutPath(workspaceLayoutPath);
    ProjectSession project(std::filesystem::temp_directory_path()/"pipeframe-workbench-ui-test");
    const auto id=project.GetDocument().CreateObject("Example object","test.object");
    project.GetDocument().MarkClean();
    std::string error;
    if(!project.GetRuntime().Load({},PIPEFRAME_TEST_RUNTIME,&error)) { std::cerr<<error; return 1; }
    const auto pluginObject=project.GetRuntime().CreateDefaultObject("test.object");
    if(pluginObject.components.size()!=2 || pluginObject.components[0].typeId!="test.sensor" ||
       pluginObject.components[1].typeId!=Transform2DComponentTypeId ||
       std::get<double>(pluginObject.components[0].properties.at("range"))!=10.0 ||
       project.GetRuntime().GetSceneComponentTypes().size()!=8) {
        std::cerr<<"Plugin component schema did not instantiate without Workbench changes.\n";
        return 1;
    }
    project.GetDocument().AddComponent(id, pluginObject.components[0]);
    project.GetDocument().SetComponentProperty(id,"test.sensor","reading",99.0);
    project.SetSelectedObject(id);
    const auto assetRoot=std::filesystem::temp_directory_path()/"pipeframe-workbench-assets";
    const auto assetSource=std::filesystem::temp_directory_path()/"pipeframe-workbench-texture.png";
    std::filesystem::remove_all(assetRoot); {std::string imageError; if(!Check(SaveImageData(assetSource, ImageData({32,32}, {80,120,160,255}), imageError), "Write real asset texture")) return 1;}
    if(!project.GetAssetDatabase().Open(assetRoot,&error)) {std::cerr<<error;return 1;}
    const auto textureAsset=project.GetAssetDatabase().ImportNow({assetSource},&error);
    if(!textureAsset) {std::cerr<<error;return 1;}
    project.SynchronizeRuntime();
    SimulationSession simulation(project.GetRuntime());
    WorkbenchInput input(view,project,simulation);
    int adds=0,buildClicks=0;
    WorkbenchView::Callbacks callbacks;
    callbacks.buildRuntime=[&]{++buildClicks;};
    callbacks.selectionChanged=[&](SceneObjectId selected) { project.SetSelectedObject(selected); view.Refresh(project,simulation,false); };
    callbacks.addObject=[&] { ++adds; project.GetDocument().CreateObject("Created while running","test.object"); project.SynchronizeRuntime(); view.Refresh(project,simulation,true); };
    callbacks.placeObject=[&](Vector2f position) { project.CreateObject(position); view.Refresh(project,simulation,true); };
    callbacks.deleteObject=[&] { project.DeleteSelectedObject(); view.Refresh(project,simulation,true); };
    callbacks.toggleSimulation=[&] { simulation.Toggle(project.GetDocument().GetObjects()); view.Refresh(project,simulation,false); };
    callbacks.singleStep=[&] { simulation.RequestSingleStep(project.GetDocument().GetObjects()); };
    callbacks.resetSimulation=[&] { simulation.Reset(project.GetDocument().GetObjects()); view.Refresh(project,simulation,false); };
    callbacks.setSimulationSpeed=[&](auto speed) { simulation.SetSpeed(speed); view.Refresh(project,simulation,false); };
    callbacks.transformCommitted=[&](const auto &transform) { project.SetSelectedTransform(transform); };
    callbacks.paintShape=[&](std::size_t layer,std::uint32_t tile,int shape){project.GetTilemapEditor().ConfigureGesture(layer,tile,static_cast<TilemapPaintShape>(shape));project.GetTilemapEditor().SetEnabled(true);view.Refresh(project,simulation,false);};
    callbacks.environmentAssets.selectBrush=[&](const std::string &id){project.GetTilemapEditor().SelectBrush(id);view.Refresh(project,simulation,false);};
    callbacks.environmentAssets.brushSetting=[&](const std::string &key,const PropertyValue &value){project.GetTilemapEditor().SetBrushSetting(key,value);view.Refresh(project,simulation,false);};
    callbacks.environmentAssets.layer=[&](const std::string &action,std::size_t layer,const std::string &name){project.GetTilemapEditor().EditLayer(action,layer,name);view.Refresh(project,simulation,false);};
    callbacks.environmentAssets.brush=[&](int radius,bool axis){project.GetTilemapEditor().SetBrushOptions(radius,axis);view.Refresh(project,simulation,false);};
    callbacks.environmentAssets.tile=[&](TileId tile,Color color,bool solid){project.GetTilemapEditor().DefineTile(tile,color,solid);view.Refresh(project,simulation,false);};
    callbacks.environmentAssets.selection=[&](TileId tile,bool boundary){project.GetTilemapEditor().PaintSelection(tile,boundary);view.Refresh(project,simulation,false);};
    callbacks.environmentAssets.save=[&]{project.GetTilemapEditor().Save();view.Refresh(project,simulation,false);};
    callbacks.environmentAssets.undo=[&]{project.GetTilemapEditor().Undo();view.Refresh(project,simulation,false);};
    callbacks.environmentAssets.redo=[&]{project.GetTilemapEditor().Redo();view.Refresh(project,simulation,false);};
    callbacks.assignAssetProperty=[&](const std::string &component,const std::string &key,const std::string &asset){project.AssignAssetToSelectedProperty(component,key,asset);};
    callbacks.assignAsset=[&](const std::string &assetId) { project.AssignAssetToFirstSelectedAssetProperty(assetId); };
    view.SetCallbacks(callbacks); view.ShowWorkspace(); view.Layout(window.getSize(),context); view.Refresh(project,simulation,true);
    input.SetOnStateChanged([&](bool rebuild) { view.Refresh(project,simulation,rebuild); });
    if(argc>1 && std::string(argv[1])=="--environment-benchmark") {
        using Clock=std::chrono::steady_clock;
        const auto ms=[](auto a,auto b){return std::chrono::duration<double,std::milli>(b-a).count();};
        const auto p95=[](std::vector<double> values){std::sort(values.begin(),values.end());return values[values.size()*95/100];};
        const auto root=std::filesystem::temp_directory_path()/"pipeframe-environment-perf";
        std::filesystem::remove_all(root);std::filesystem::create_directories(root);
        if(!project.GetAssetDatabase().Open(root,&error))return 1;
        bool gates=true;
        sf::RenderTexture target({1440,900});auto capture=pipeframe::backend::sfml::MakeRenderContext(target);
        view.Layout({1440,900},capture);
        for(const auto size:{Vector2i{384,216},Vector2i{1024,1024}}){
            Tilemap2D map(size.x,size.y);map.AddLayer("Terrain");map.DefineTile({1,{90,100,110,255},true});
            for(int y=0;y<size.y;++y)for(int x=0;x<size.x;++x)if((x+7*y)%5==0)map.SetTile(0,{x,y},1);
            const auto source=root/(std::to_string(size.x)+".pftilemap");
            {std::ofstream out(source);TilemapSerializer::Save(map,out);}
            const auto loaded=Clock::now();auto asset=project.GetAssetDatabase().ImportNow({source},&error);
            if(!asset||!project.BeginTilemapEditing(*asset)){std::cerr<<error;return 1;}
            capture.GetCamera().SetCenter({size.x*.5f,size.y*.5f});capture.GetCamera().SetSize({size.x*1.25f,size.y*1.6f});capture.GetCamera().SetZoom(1);
            const auto frame=[&]{target.clear();capture.BeginWorld();view.RenderWorldBackground(capture);project.RenderTilemapEditing(capture);view.RenderWorldOverlay(capture);capture.BeginScreen();view.RenderScreen(capture,simulation);target.display();};
            view.Refresh(project,simulation,true);view.Update(.016f);frame();
            const auto loadMs=ms(loaded,Clock::now());
            std::vector<double> frames,inputs;double uiTotal=0,drawTotal=0;
            std::ofstream csv;if(argc>2){std::filesystem::create_directories(argv[2]);csv.open(std::filesystem::path(argv[2])/("map-"+std::to_string(size.x)+".csv"));csv<<"frame,input_ms,ui_ms,draw_ms,total_ms\n";}
            for(int i=0;i<130;++i){
                project.GetTilemapEditor().ConfigureGesture(0,i%2?0:1,TilemapPaintShape::Pencil);project.GetTilemapEditor().SetBrushOptions(4,false);
                const auto pixel=capture.MapWorldToPixel({float(32+(i%20)*8),float(32+(i/20)*8)});
                const auto start=Clock::now();
                if(!project.HandleTilemapEvent({InputEventType::PointerPressed,PointerInput{PointerButton::Left,pixel}},capture))return 1;
                if(!project.HandleTilemapEvent({InputEventType::PointerReleased,PointerInput{PointerButton::Left,pixel}},capture))return 1;
                const auto inputEnd=Clock::now();view.Refresh(project,simulation,false);view.Update(1.f/60.f);const auto uiEnd=Clock::now();frame();const auto end=Clock::now();
                if(i>=10){frames.push_back(ms(start,end));inputs.push_back(ms(start,inputEnd));uiTotal+=ms(inputEnd,uiEnd);drawTotal+=ms(uiEnd,end);
                    if(csv)csv<<i-10<<','<<inputs.back()<<','<<ms(inputEnd,uiEnd)<<','<<ms(uiEnd,end)<<','<<frames.back()<<'\n';}
            }
            const auto saving=Clock::now();if(!project.GetTilemapEditor().Save())return 1;const auto saveMs=ms(saving,Clock::now());
#if defined(__unix__) || defined(__APPLE__)
            rusage usage{};getrusage(RUSAGE_SELF,&usage);
#ifdef __APPLE__
            const double memory=double(usage.ru_maxrss)/(1024*1024);
#else
            const double memory=double(usage.ru_maxrss)/1024;
#endif
#else
            const double memory=std::numeric_limits<double>::infinity();
#endif
            std::sort(frames.begin(),frames.end());
            std::cout<<size.x<<'x'<<size.y<<"; median "<<frames[60]<<"; p95 "<<p95(frames)<<"; input p95 "<<p95(inputs)<<"; mean UI/draw "<<uiTotal/120<<' '<<drawTotal/120<<"; load "<<loadMs<<"; save "<<saveMs<<"; peak MiB "<<memory<<"; source bytes "<<std::filesystem::file_size(source)<<std::endl;
            gates &= p95(frames)<=16.67&&p95(inputs)<=4&&loadMs<=1000&&saveMs<=1000&&memory<=512;
            project.GetTilemapEditor().Close(true);
        }
        return gates?0:2;
    }
    if (argc>1 && std::string(argv[1])=="--ui-benchmark") {
        for(int i=0;i<10;++i) { view.Refresh(project,simulation,false); view.Update(.016f); }
        std::vector<double> samples;
        for(int i=0;i<120;++i) {
            const auto start=std::chrono::steady_clock::now();
            view.Refresh(project,simulation,false); view.Update(.016f);
            samples.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
        }
        std::sort(samples.begin(),samples.end());
        std::cout<<"Paused selected Inspector refresh/update (no GPU/present), 120 frames: median "<<samples[60]<<" ms, p95 "<<samples[114]<<" ms\n";
        return 0;
    }

    auto click=[&](Widget &widget) {
        auto p=widget.GetScreenPosition()+widget.GetSize()*0.5f; sf::Vector2i pixel{static_cast<int>(p.x),static_cast<int>(p.y)};
        DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,pixel},context);
        DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,pixel},context);
    };
    const auto componentProperty=[](const SceneObjectData *object,const std::string &componentId,
                                    const std::string &key)->const PropertyValue & {
        return std::ranges::find(object->components,componentId,&SceneComponentData::typeId)->properties.at(key);
    };
    bool passed=true;
    auto &shell=view.GetShell();
    auto *buildButton=FindAction(shell.Toolbar(),"BUILD & RELOAD");
    passed &= Check(buildButton!=nullptr,"Build action exists in the toolbar");if(buildButton)click(*buildButton);
    passed &= Check(buildClicks==1,"Build button dispatches build workflow");
    view.SetBuildProgress(true,"Compiling test source...");view.Update(.016f);
    auto *cancelBuild=FindAction(shell.Toolbar(),"CANCEL BUILD");
    passed &= Check(cancelBuild!=nullptr,"Running build exposes cancellation");if(cancelBuild)click(*cancelBuild);
    passed &= Check(buildClicks==2,"Cancel button dispatches cancellation through active workflow");
    view.SetBuildProgress(false,"Build and reload succeeded.");view.Update(.016f);

    if(auto *scroll=FindScroll(shell.Inspector())) {
        const auto bounds=scroll->GetBounds();
        for(const auto offset:{sf::Vector2f{2,2},sf::Vector2f{6,80},sf::Vector2f{bounds.size.x-3,100},sf::Vector2f{60,80}}) {
            scroll->SetScrollOffset(0);
            const auto point=scroll->GetScreenPosition()+offset;
            view.HandleEvent(sf::Event::MouseWheelScrolled{sf::Mouse::Wheel::Vertical,-1,sf::Vector2i(point)});
            passed &= Check(scroll->GetScrollOffset()>0,"Inspector padding and blank space must accept wheel scrolling");
        }
        scroll->SetScrollOffset(0);
    } else passed &= Check(false,"Inspector has a scroll viewport");
    const auto sections=shell.Inspector().GetVisibleComponentSections();
    const auto keys=shell.Inspector().GetVisiblePropertyKeys();
    passed &= Check(std::ranges::find(sections,"COMPONENT  |  Transform")!=sections.end() &&
                    std::ranges::find(sections,"COMPONENT  |  Test Sensor")!=sections.end(),
                    "Inspector must generate shared and plugin component sections from metadata");
    passed &= Check(keys.size()>=13 && std::ranges::find(keys,"mode")!=keys.end() &&
                    std::ranges::find(keys,"reading")!=keys.end(),
                    "Inspector must materialize every standard plugin property kind including telemetry");
    const auto liveReading=shell.Inspector().GetDisplayedPropertyValue("test.sensor","reading");
    passed &= Check(liveReading && std::get<double>(*liveReading)==0.0 &&
                    std::get<double>(componentProperty(project.GetDocument().FindObject(id),"test.sensor","reading"))==99.0,
                    "Live telemetry must override display without mutating authored scene data");
    const auto sensorPresentation=shell.Inspector().GetCustomPresentation("test.sensor","range");
    passed &= Check(sensorPresentation&&sensorPresentation->detail=="PROJECT PREVIEW"&&
                        sensorPresentation->normalizedValue==0.1,
                    "A plugin-defined Inspector drawer must execute through its registered editor hint");
    auto *sensorFoldout=FindAction(shell.Inspector(),"v  COMPONENT  |  Transform");
    passed &= Check(sensorFoldout!=nullptr,"Generated component sections must expose a foldout control");
    if(sensorFoldout) click(*sensorFoldout);
    passed &= Check(shell.Inspector().IsSectionCollapsed(Transform2DComponentTypeId),
                    "A generated component foldout must collapse its property controls");
    sensorFoldout=FindAction(shell.Inspector(),">  COMPONENT  |  Transform");
    if(sensorFoldout) click(*sensorFoldout);
    passed &= Check(!shell.Inspector().IsSectionCollapsed(Transform2DComponentTypeId),
                    "A generated component foldout must restore its property controls");
    passed &= Check(shell.Assets().GetVisibleAssetCount()==1,
                    "The Asset Browser must display imported project assets");
    auto *assetsButton=FindAction(shell.Toolbar(),"ASSETS");
    passed &= Check(assetsButton!=nullptr,"The workspace must expose the Asset Browser");
    if(assetsButton)click(*assetsButton);
    passed &= Check(view.IsAssetBrowserVisible()&&shell.Assets().IsVisible()&&!shell.Inspector().IsVisible(),
                    "The Asset Browser must replace the Inspector without overlapping the viewport");
    for(const auto *category : {"SHADERS", "MAPS", "TEXTURES"}) {
        auto *tab=FindAction(shell.Assets(),category);
        passed &= Check(tab!=nullptr,"Asset categories must be directly clickable");
        if(tab)click(*tab);
        passed &= Check(shell.Assets().GetVisibleAssetCount()==(std::string(category)=="TEXTURES"?1u:0u),
                        "Category tabs must filter actual asset types");
    }
    auto *assetRow=FindAction(shell.Assets(),"pipeframe-workbench-texture.png | Texture | Ready");
    passed &= Check(assetRow!=nullptr,"Imported assets must be selectable from the browser");
    if(assetRow)click(*assetRow);
    auto *assignAsset=FindAction(shell.Assets(),"ASSIGN TO "+project.FindComponentType("test.sensor")->displayName+" / Texture"); passed &= Check(assignAsset!=nullptr,"Explicit compatible assignment button"); if(assignAsset)click(*assignAsset);
    passed &= Check(std::get<AssetReference>(componentProperty(project.GetDocument().FindObject(id),"test.sensor","texture")).assetId==*textureAsset,
                    "Asset Browser assignment must route through the typed Inspector property transaction");
    project.GetDocument().SetComponentProperty(id,"test.sensor","texture",AssetReference{});
    if(assetRow){
        const auto from=sf::Vector2i(assetRow->GetScreenPosition()+assetRow->GetSize()*0.5f);
        const auto to=sf::Vector2i(shell.Viewport().GetScreenPosition()+shell.Viewport().GetSize()*0.5f);
        DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,from},context);
        DispatchInput(input,sf::Event::MouseMoved{to},context);
        passed &= Check(view.IsPlacementPreviewVisible(),
                        "Dragging an asset over the viewport must show a placement preview");
        DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,to},context);
    }
    passed &= Check(!view.IsPlacementPreviewVisible(),"Completing an asset drop must clear its placement preview");
    passed &= Check(std::get<AssetReference>(componentProperty(project.GetDocument().FindObject(id),"test.sensor","texture")).assetId==*textureAsset,
                    "Dragging an asset onto the viewport must use the same typed assignment transaction");
    assetsButton=FindAction(shell.Toolbar(),"INSPECT"); if(assetsButton)click(*assetsButton);
    passed &= Check(!view.IsAssetBrowserVisible()&&shell.Inspector().IsVisible(),
                    "The Inspector must remain reachable after browsing assets");
    passed &= Check(!project.AssignAssetToSelectedProperty("test.sensor","texture","missing"),
                    "Asset assignment must accept only ready stable database IDs");
    project.GetDocument().SetComponentProperty(id,"test.sensor","texture",AssetReference{"missing"});
    passed &= Check(project.FindMissingAssetReferences()==std::vector<std::string>{"missing"} &&
                    project.RepairAssetReferences("missing",*textureAsset) &&
                    std::get<AssetReference>(componentProperty(project.GetDocument().FindObject(id),"test.sensor","texture")).assetId==*textureAsset,
                    "Missing-reference repair must update authored references transactionally");
    project.Undo();
    for(auto size:{sf::Vector2u{640,800},sf::Vector2u{1280,720},sf::Vector2u{1920,1080},
                  sf::Vector2u{2560,1440},sf::Vector2u{3440,1440}}) {
        sf::RenderWindow captureWindow(sf::VideoMode(size),"Workbench snapshot");captureWindow.setVisible(false);
        auto captureContext = pipeframe::backend::sfml::MakeRenderContext(captureWindow);view.Layout(size,captureContext);
        const auto bounds=shell.GetWorldBounds();
        passed &= Check(bounds.size.x>200 && bounds.size.y>180,"Responsive shell must retain a usable viewport");
        passed &= Check(!bounds.findIntersection(shell.Inspector().GetBounds()) && !bounds.findIntersection(shell.Hierarchy().GetBounds()),"Tools must not overlap the world viewport");
    }
    window.setSize({1100,800}); context.SetScreenSize({1100,800}); view.Layout({1100,800},context);
    auto *dockSplitter = FindSplitter(shell);
    passed &= Check(dockSplitter != nullptr, "Workbench must use the engine dock host");
    if (dockSplitter) {
        const auto before = shell.GetWorldBounds().size.x;
        const auto start = sf::Vector2i(dockSplitter->GetScreenPosition()+dockSplitter->GetSize()*0.5f);
        view.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,start});
        view.HandleEvent(sf::Event::MouseMoved{start+sf::Vector2i{-30,0}});
        view.HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,start+sf::Vector2i{-30,0}});
        view.Layout({1100,800},context);
        passed &= Check(shell.GetWorldBounds().size.x < before && !dockSplitter->IsDragging(),
                        "Engine dock controller must resize the live editor and release capture");
        shell.ResetWorkspaceLayout(); view.Layout({1100,800},context);
    }
    auto *panelsButton=FindAction(shell.Toolbar(),"PANELS");
    passed &= Check(panelsButton!=nullptr,"The standard tool panels must be discoverable from the toolbar");
    passed &= Check(FindAction(shell.Toolbar(),"RELOAD")!=nullptr,"Project runtime hot reload must be discoverable from the toolbar");
    if(panelsButton)click(*panelsButton);
    passed &= Check(shell.IsBottomDockVisible()&&shell.BottomTools().GetTabCount()==8&&
                        !shell.GetWorldBounds().findIntersection(shell.BottomTools().GetBounds()),
                    "Standard and project-extension tool tabs must dock without covering the viewport");
    const char *toolTabs[]{"CONSOLE","PROFILE","LEARN","GAME","CONNECT","TELEM","COMMANDS","EXTEND"};
    for(std::size_t tab=0;tab<8;++tab) {
        auto *button=FindAction(shell.BottomTools(),toolTabs[tab]);
        passed &= Check(button!=nullptr,"Every tool tab has a mounted action");
        if(button)click(*button);
        passed &= Check(shell.BottomTools().GetSelectedTab()==tab,"Tool tab click changes displayed content");
    }
    passed &= Check(project.GetRuntime().GetExtensions().All().size()==17&&
                        project.GetRuntime().GetActions().All().size()==1&&
                        project.GetRuntime().GetSystems().All().size()==1,
                    "An external runtime must register every extension family, commands, and systems without Workbench changes");
    for(const auto &extension:project.GetRuntime().GetExtensions().All())
        passed &= Check(extension.createBrush?bool(extension.createBrush()):project.GetRuntime().InvokeExtension(extension.id,{},&error),
                        "Every extension must execute through its registered action or brush factory");
    panelsButton=FindAction(shell.Toolbar(),"PANELS ON"); if(panelsButton)click(*panelsButton);
    passed &= Check(!shell.IsBottomDockVisible(),"The bottom tool dock must close without leaving an input surface behind");
    view.SetToolsFloating(true);
    const auto floatingBefore=view.GetFloatingToolsBounds();
    passed &= Check(view.AreToolsFloating()&&!shell.IsBottomDockVisible()&&floatingBefore.size.x>=360&&floatingBefore.size.y>=240,
                    "The standard tool suite must float as a bounded, usable workspace window");
    const sf::Vector2i dragStart(floatingBefore.position+sf::Vector2f{80,20});
    view.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,dragStart});
    view.HandleEvent(sf::Event::MouseMoved{dragStart+sf::Vector2i{45,30}});
    view.HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,dragStart+sf::Vector2i{45,30}});
    const auto floatingAfter=view.GetFloatingToolsBounds();
    passed &= Check(floatingAfter.position!=floatingBefore.position&&
                        floatingAfter.position.x>=0&&floatingAfter.position.y>=0&&
                        floatingAfter.position.x+floatingAfter.size.x<=1100&&
                        floatingAfter.position.y+floatingAfter.size.y<=800,
                    "A floating tool window must drag interactively and remain reachable on screen");
    passed &= Check(shell.LoadWorkspaceLayout(&error)&&shell.AreStandardToolsFloating()&&
                        shell.GetStandardToolsFloatingBounds()==floatingAfter,
                    "Floating mode and bounds must survive a workspace-layout round trip");
    const sf::Vector2i grip(floatingAfter.position+floatingAfter.size-sf::Vector2f{5,5});
    view.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,grip});
    view.HandleEvent(sf::Event::MouseMoved{grip+sf::Vector2i{40,30}});
    view.HandleEvent(sf::Event::FocusLost{});
    const auto resized=view.GetFloatingToolsBounds();
    view.HandleEvent(sf::Event::MouseMoved{grip+sf::Vector2i{90,80}});
    passed &= Check(resized.size.x>floatingAfter.size.x && resized.size.y>floatingAfter.size.y &&
                    view.GetFloatingToolsBounds()==resized,
                    "Floating resize must use the shared controller and stop on focus loss");
    view.SetToolsFloating(false);
    passed &= Check(!view.AreToolsFloating()&&shell.IsBottomDockVisible(),
                    "A floating workspace window must dock back into the split layout");
    view.ToggleStandardPanels();
    DispatchInput(input,sf::Event::KeyPressed{sf::Keyboard::Key::F7},context);
    passed &= Check(view.IsAssetBrowserVisible(),"F7 must make the Asset Browser reachable from the keyboard");
    DispatchInput(input,sf::Event::KeyPressed{sf::Keyboard::Key::F7},context);
    DispatchInput(input,sf::Event::KeyPressed{sf::Keyboard::Key::F8},context);
    passed &= Check(shell.IsBottomDockVisible(),"F8 must make the standard tool dock reachable from the keyboard");
    DispatchInput(input,sf::Event::KeyPressed{sf::Keyboard::Key::F8},context);
    window.setSize({640,800}); context.SetScreenSize({640,800}); view.Layout({640,800},context);
    panelsButton=FindAction(shell.Toolbar(),"PANELS"); if(panelsButton)click(*panelsButton);
    passed &= Check(view.HasBlockingOverlay()&&!shell.IsBottomDockVisible(),
                    "A compact workspace must open standard tools as one modal panel instead of covering the viewport with docks");
    DispatchInput(input,sf::Event::KeyPressed{sf::Keyboard::Key::Escape},context);
    view.Update(0.3f); view.Update(0.01f);
    passed &= Check(!view.HasBlockingOverlay(),"Escape must dismiss the compact standard-tools panel");
    window.setSize({1100,800}); context.SetScreenSize({1100,800}); view.Layout({1100,800},context);
    simulation.Toggle(project.GetDocument().GetObjects()); simulation.FixedUpdate(0.016f);
    passed &= Check(project.GetRuntime().GetStatistics().usingQuads,
                    "A registered scheduled system must submit backend-neutral viewport debug geometry");
    project.GetDocument().SetPrefabLinks(id, {
        {"wheel", 3, 7, id, {}},
        {"robot", 5, 2, id, "robot.base"},
    });
    project.GetDocument().SetComponentProperty(id,"test.sensor","range",17.5);
    const auto authoredComponentBeforeReload=*std::ranges::find(
        project.GetDocument().FindObject(id)->components,"test.sensor",
        &SceneComponentData::typeId);
    const auto prefabLinksBeforeReload=project.GetDocument().FindObject(id)->prefabLinks;
    project.SynchronizeRuntime();
    const auto ticksBeforeReload=project.GetRuntime().GetStatistics().visibleObjectCount;
    passed &= Check(project.GetRuntime().Reload(project.GetDocument().GetObjects(),project.GetSelectedObjectId(),&error)&&
                        project.GetRuntime().GetStatistics().visibleObjectCount==ticksBeforeReload&&
                        project.GetRuntime().GetExtensions().All().size()==17,
                    "Hot reload must preserve runtime state, scene selection, and extension registrations");
    const auto *reloadedObject=project.GetDocument().FindObject(id);
    const auto reloadedTelemetry=project.GetRuntime().GetLiveComponentProperties();
    const auto reloadedPrefabCount=std::ranges::find_if(reloadedTelemetry,[&](const auto &value){
        return value.objectId==id&&value.componentTypeId=="test.reload-proof"&&
               value.propertyKey=="prefabLinkCount";
    });
    passed &= Check(reloadedObject&&reloadedObject->prefabLinks==prefabLinksBeforeReload&&
                        *std::ranges::find(reloadedObject->components,"test.sensor",
                                           &SceneComponentData::typeId)==authoredComponentBeforeReload&&
                        reloadedObject->prefabLinks[0].prefabId=="wheel"&&
                        reloadedObject->prefabLinks[1].prefabId=="robot"&&
                        std::get<double>(componentProperty(reloadedObject,"test.sensor","range"))==17.5&&
                        reloadedPrefabCount!=reloadedTelemetry.end()&&
                        std::get<std::int64_t>(reloadedPrefabCount->value)==2,
                    "Nested prefab identity and component overrides must survive plugin reload in both the scene and replacement runtime");
    project.GetDocument().SetPrefabLinks(id,{});
    project.GetDocument().SetComponentProperty(id,"test.sensor","range",10.0);
    project.SynchronizeRuntime();
    const auto missingRuntime=std::filesystem::temp_directory_path()/"missing-pipeframe-runtime";
    passed &= Check(!project.GetRuntime().ReloadFrom(missingRuntime,project.GetDocument().GetObjects(),
                                                     project.GetSelectedObjectId(),&error)&&
                        project.GetRuntime().HasRuntime()&&
                        project.GetRuntime().GetStatistics().visibleObjectCount==ticksBeforeReload,
                    "A failed reload must leave the active runtime and state untouched");
    error.clear();
    passed &= Check(!project.GetRuntime().ReloadFrom(PIPEFRAME_OLD_ABI_RUNTIME,
        project.GetDocument().GetObjects(), project.GetSelectedObjectId(), &error) &&
        error.find("ABI") != std::string::npos && project.GetRuntime().HasRuntime(),
        "Old render-context ABI is rejected before registration and preserves the live runtime");
    { ProjectRuntimeHost oldHost; error.clear();
      passed &= Check(!oldHost.Load({},PIPEFRAME_OLD_ABI_RUNTIME,&error) &&
        error.find("ABI") != std::string::npos && !oldHost.HasRuntime(),
        "Initial load rejects old render-context ABI with a rebuild diagnostic"); }
    passed &= Check(!project.GetRuntime().ReloadFrom(PIPEFRAME_INCOMPATIBLE_RUNTIME,
                                                     project.GetDocument().GetObjects(),
                                                     project.GetSelectedObjectId(),&error)&&
                        project.GetRuntime().HasRuntime()&&
                        project.GetRuntime().GetStatistics().visibleObjectCount==ticksBeforeReload,
                    "A partially registered replacement must unload safely without invalidating plugin callbacks");
    view.SetWorkspaceMode(WorkbenchWorkspaceMode::Simulation); view.Refresh(project,simulation,true);
    passed &= Check(shell.IsGameViewportSelected()&&shell.Hierarchy().IsVisible() && shell.Inspector().IsVisible() && simulation.CanAuthorScene(),"Simulation viewport tab must retain authoring tools");
    auto *add=FindAction(shell.Hierarchy(),"+"); passed &= Check(add && add->IsEnabled(),"Add must remain enabled while playing"); if(add) click(*add);
    simulation.FixedUpdate(0.016f);
    passed &= Check(adds==1 && project.GetRuntime().GetStatistics().candidateObjectCount==2 && project.GetRuntime().GetStatistics().visibleObjectCount==2,"Live edits must synchronize and resume the running runtime");
    const auto point=pipeframe::backend::sfml::WorldToScreen(context,{0,0});
    DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Right,point},context);
    DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Right,point},context);
    passed &= Check(project.GetSelectedObjectId()==id && view.HasBlockingOverlay(),"Right click must select and open a context menu");
    const auto count=project.GetDocument().GetObjects().size();
    DispatchInput(input,sf::Event::KeyPressed{sf::Keyboard::Key::Delete},context);
    passed &= Check(project.GetDocument().GetObjects().size()==count,"Context menu must block destructive world shortcuts");
    DispatchInput(input,sf::Event::KeyPressed{sf::Keyboard::Key::Escape},context); view.Update(0.3f); view.Update(0.01f);
    passed &= Check(!view.HasBlockingOverlay(),"Escape must dismiss the context menu");
    DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,point},context);
    DispatchInput(input,sf::Event::MouseMoved{point+sf::Vector2i{20,10}},context);
    DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,point+sf::Vector2i{20,10}},context);
    passed &= Check(project.GetSelectedObject()->transform.position.x!=0 && project.CanUndo(),"Dragging during simulation must commit an undoable edit");
    project.Undo();
    passed &= Check(project.GetSelectedObject()->transform.position==pipeframe::Vector2f{},"Undo must restore the live authored transform");
    auto *physicsToggle=FindAction(shell.Toolbar(),"PHYSICS OFF");
    passed &= Check(physicsToggle!=nullptr,"Shared physics visualization action exists");
    if(physicsToggle)click(*physicsToggle);
    passed &= Check(view.GetWorldDebugOptions().physics,"Physics button enables debug geometry");
    physicsToggle=FindAction(shell.Toolbar(),"PHYSICS ON");if(physicsToggle)click(*physicsToggle);
    auto *meshToggle=FindAction(shell.Toolbar(),"MESH OFF");
    passed &= Check(meshToggle!=nullptr,"Shared mesh visualization action exists");
    if(meshToggle)click(*meshToggle);
    passed &= Check(view.GetWorldDebugOptions().mesh&&!view.GetWorldDebugOptions().physics,"Debug channels toggle independently");
    meshToggle=FindAction(shell.Toolbar(),"MESH ON");if(meshToggle)click(*meshToggle);
    auto *gridToggle=FindAction(shell.Toolbar(),"GRID ON");
    passed &= Check(gridToggle!=nullptr && view.IsGridVisible(),"The adaptive editor grid must start visible");
    if(gridToggle) click(*gridToggle);
    passed &= Check(!view.IsGridVisible(),"The toolbar must hide the editor grid");
    if(gridToggle) click(*gridToggle);
    passed &= Check(view.IsGridVisible(),"The toolbar must restore the editor grid");
    auto *gridStep=FindAction(shell.Toolbar(),"GRID AUTO");
    passed &= Check(gridStep!=nullptr&&view.GetGridStep()==0.0f,"Grid spacing must start in adaptive mode");
    if(gridStep)click(*gridStep);
    view.RenderWorldBackground(context);
    passed &= Check(std::abs(view.GetGridStep()-1.0f)<0.001f&&view.GetDisplayedGridStep()>=1.0f,
                    "The visible grid spacing control must select a deterministic authored step");
    auto *snapToggle=FindAction(shell.Toolbar(),"POS OFF");
    passed &= Check(snapToggle!=nullptr && !view.IsPositionSnapEnabled(),"Position snapping must be an explicit editor mode");
    if(snapToggle) click(*snapToggle);
    passed &= Check(view.IsPositionSnapEnabled(),"The SNAP control must enable position snapping");
    const auto snappedTarget=pipeframe::backend::sfml::WorldToScreen(context,{13,17});
    DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,point},context);
    DispatchInput(input,sf::Event::MouseMoved{snappedTarget},context);
    DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,snappedTarget},context);
    passed &= Check(project.GetSelectedObject()->transform.position==pipeframe::Vector2f{10,20},
                    "A viewport drag must snap both position axes to the visible ten-unit step");
    project.Undo();
    if(snapToggle) click(*snapToggle);
    passed &= Check(!view.IsPositionSnapEnabled(),"The SNAP control must return to free dragging");
    view.SetGridOrigin({3,7});
    if(snapToggle)click(*snapToggle);
    const auto originSnapTarget=pipeframe::backend::sfml::WorldToScreen(context,{14,19});
    DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,point},context);
    DispatchInput(input,sf::Event::MouseMoved{originSnapTarget},context);
    DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,originSnapTarget},context);
    passed &= Check(project.GetSelectedObject()->transform.position==pipeframe::Vector2f{13,17},
                    "Position snapping must use the user-defined grid origin");
    project.Undo();if(snapToggle)click(*snapToggle);view.SetGridOrigin({});

    view.SetTransformTool(TransformTool::Move);view.Refresh(project,simulation,false);
    const auto xMoveHandle=point+sf::Vector2i{46,0};
    DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,xMoveHandle},context);
    DispatchInput(input,sf::Event::MouseMoved{xMoveHandle+sf::Vector2i{24,18}},context);
    DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,xMoveHandle+sf::Vector2i{24,18}},context);
    passed &= Check(std::abs(project.GetSelectedObject()->transform.position.x)>0.01f&&
                        std::abs(project.GetSelectedObject()->transform.position.y)<0.01f,
                    "The move X handle must constrain a direct drag to its visible axis");
    project.Undo();view.Refresh(project,simulation,false);
    for(const float zoom:{0.5f,2.0f}){
        context.GetCamera().SetZoom(zoom);
        const auto zoomedPivot=pipeframe::backend::sfml::WorldToScreen(context,{0,0});
        const auto zoomedHandle=pipeframe::backend::sfml::ScreenToWorld(context,zoomedPivot+sf::Vector2i{46,0});
        passed &= Check(view.HitTestTransformHandle(zoomedHandle)==TransformHandle::MoveX,
                        "Transform handles must retain screen-space hit targets across camera/DPI scale");
    }
    context.GetCamera().SetZoom(1.0f);

    auto *angleSnap=FindAction(shell.Toolbar(),"ANG OFF");
    passed &= Check(angleSnap!=nullptr && !view.IsRotationSnapEnabled(),"Angle snapping must start disabled");
    if(angleSnap) click(*angleSnap);
    view.SetTransformTool(TransformTool::Rotate);
    DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,point},context);
    DispatchInput(input,sf::Event::MouseMoved{point+sf::Vector2i{0,100}},context);
    DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,point+sf::Vector2i{0,100}},context);
    passed &= Check(std::abs(project.GetSelectedObject()->transform.rotation-90.0f)<0.01f,
                    "Rotate gizmo mode must apply the configured angle increment");
    project.Undo();

    view.Refresh(project,simulation,false);
    const auto rotateHandle=point+sf::Vector2i{42,0};
    DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,rotateHandle},context);
    DispatchInput(input,sf::Event::MouseMoved{point+sf::Vector2i{0,42}},context);
    DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,point+sf::Vector2i{0,42}},context);
    passed &= Check(std::abs(project.GetSelectedObject()->transform.rotation-90.0f)<0.01f,
                    "The visible rotation ring must be directly hit-testable");
    project.Undo();

    auto *scaleSnap=FindAction(shell.Toolbar(),"SCL OFF");
    passed &= Check(scaleSnap!=nullptr && !view.IsScaleSnapEnabled(),"Scale snapping must start disabled");
    if(scaleSnap) click(*scaleSnap);
    view.SetTransformTool(TransformTool::Scale);
    DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,point},context);
    DispatchInput(input,sf::Event::MouseMoved{point+sf::Vector2i{25,0}},context);
    DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,point+sf::Vector2i{25,0}},context);
    passed &= Check(project.GetSelectedObject()->transform.scale.x>1.0f && project.GetSelectedObject()->transform.scale.x==project.GetSelectedObject()->transform.scale.y,
                    "Scale gizmo mode must update both axes through one transaction");
    project.Undo();
    view.Refresh(project,simulation,false);
    const auto xScaleHandle=point+sf::Vector2i{46,0};
    DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,xScaleHandle},context);
    DispatchInput(input,sf::Event::MouseMoved{xScaleHandle+sf::Vector2i{30,15}},context);
    DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,xScaleHandle+sf::Vector2i{30,15}},context);
    passed &= Check(project.GetSelectedObject()->transform.scale.x>1.0f&&
                        std::abs(project.GetSelectedObject()->transform.scale.y-1.0f)<0.001f,
                    "The scale X handle must change one axis without changing the other");
    project.Undo();
    view.SetTransformTool(TransformTool::Move);

    const auto secondId=project.GetDocument().CreateObject("Second object","test.object",{{100,0},0,{1,1}});
    auto secondSensor = pluginObject.components[0];
    secondSensor.properties.insert_or_assign("range", 25.0);
    secondSensor.properties.insert_or_assign("offset", Vector2f{3,4});
    project.GetDocument().AddComponent(secondId, secondSensor);
    project.GetDocument().AddConnection({1,SceneConnectionKind::Signal,{id,"signal"},{secondId,"signal"}});
    project.GetDocument().SetComponentProperty(id, "test.sensor", "offset", Vector2f{1,2});
    project.SetSelectedObjects({id,secondId});
    view.Refresh(project,simulation,true);
    passed &= Check(view.GetAttachmentPreviewCount()==2&&view.GetConnectionPreviewCount()==1,
                    "Registered attachment metadata and authored connections must produce viewport previews");
    context.BeginWorld(); view.RenderWorldOverlay(context);
    passed &= Check(shell.Inspector().IsPropertyMixed("test.sensor","range"),
                    "Multi-selection must show mixed component values");
    passed &= Check(!project.SetSelectedComponentProperty("test.sensor","range",101.0) &&
                    !project.SetSelectedComponentProperty("test.sensor","reading",1.0) &&
                    !project.SetSelectedComponentProperty("test.sensor","mode",std::string{"Invalid"}),
                    "Inspector transactions must enforce range, read-only, and enum metadata");
    passed &= Check(project.SetSelectedComponentProperty("test.sensor","range",42.0) &&
                    std::get<double>(componentProperty(project.GetDocument().FindObject(id),"test.sensor","range"))==42.0 &&
                    std::get<double>(componentProperty(project.GetDocument().FindObject(secondId),"test.sensor","range"))==42.0,
                    "A component edit must apply to the complete selection");
    passed &= Check(project.Undo() &&
                    std::get<double>(componentProperty(project.GetDocument().FindObject(id),"test.sensor","range"))==10.0 &&
                    std::get<double>(componentProperty(project.GetDocument().FindObject(secondId),"test.sensor","range"))==25.0,
                    "A batch property edit must undo as one transaction");
    passed &= Check(project.SetSelectedComponentProperty("test.sensor","offset",Vector2f{9,4}) &&
                    std::get<Vector2f>(componentProperty(project.GetDocument().FindObject(id),"test.sensor","offset"))==Vector2f{9,2} &&
                    std::get<Vector2f>(componentProperty(project.GetDocument().FindObject(secondId),"test.sensor","offset"))==Vector2f{9,4},
                    "Editing one mixed vector axis must preserve each object's other axis");
    project.Undo();
    project.SetSelectedObject(id);
    const auto copiedSensor=project.CopySelectedComponent("test.sensor");
    project.SetSelectedObject(secondId);
    passed &= Check(copiedSensor && project.PasteComponentToSelected(*copiedSensor) &&
                    std::get<double>(componentProperty(project.GetDocument().FindObject(secondId),"test.sensor","range"))==10.0,
                    "Component copy/paste must use typed component data without editor-specific branches");
    passed &= Check(project.Undo() && project.ResetSelectedComponentProperty("test.sensor","range") &&
                    std::get<double>(componentProperty(project.GetDocument().FindObject(secondId),"test.sensor","range"))==10.0,
                    "Property reset must apply the registered default and remain undoable");
    project.Undo();
    project.SynchronizeRuntime(); view.Refresh(project,simulation,true);
    const auto boxStart=pipeframe::backend::sfml::WorldToScreen(context,{-70,-70});
    const auto boxEnd=pipeframe::backend::sfml::WorldToScreen(context,{170,70});
    DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,boxStart},context);
    DispatchInput(input,sf::Event::MouseMoved{boxEnd},context);
    DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,boxEnd},context);
    passed &= Check(project.GetSelectedObjectIds().size()>=2 && project.IsObjectSelected(id) && project.IsObjectSelected(secondId),
                    "Box selection must select every authored object inside its world bounds");
    const auto firstBefore=project.GetDocument().FindObject(id)->transform.position;
    const auto secondBefore=project.GetDocument().FindObject(secondId)->transform.position;
    DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,point},context);
    DispatchInput(input,sf::Event::MouseMoved{point+sf::Vector2i{20,10}},context);
    DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,point+sf::Vector2i{20,10}},context);
    const auto firstDelta=project.GetDocument().FindObject(id)->transform.position-firstBefore;
    const auto secondDelta=project.GetDocument().FindObject(secondId)->transform.position-secondBefore;
    passed &= Check(std::abs(firstDelta.x-secondDelta.x)<0.001f &&
                        std::abs(firstDelta.y-secondDelta.y)<0.001f,
                    "Moving a multi-selection must preserve relative placement");
    project.Undo();project.SetSelectedObjects({id,secondId});view.Refresh(project,simulation,false);
    view.SetTransformTool(TransformTool::Scale);
    const auto selectionPivotScreen=pipeframe::backend::sfml::WorldToScreen(context,{50,0});
    const auto selectionScaleHandle=selectionPivotScreen+sf::Vector2i{46,0};
    DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,selectionScaleHandle},context);
    DispatchInput(input,sf::Event::MouseMoved{selectionScaleHandle+sf::Vector2i{50,0}},context);
    DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,selectionScaleHandle+sf::Vector2i{50,0}},context);
    passed &= Check(project.GetDocument().FindObject(id)->transform.position.x<0&&
                        project.GetDocument().FindObject(secondId)->transform.position.x>100,
                    "Center pivot scaling must transform the selected objects around their shared center");
    project.Undo();view.CyclePivotMode();view.Refresh(project,simulation,false);
    DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,selectionScaleHandle},context);
    DispatchInput(input,sf::Event::MouseMoved{selectionScaleHandle+sf::Vector2i{50,0}},context);
    DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,selectionScaleHandle+sf::Vector2i{50,0}},context);
    passed &= Check(project.GetDocument().FindObject(id)->transform.position==Vector2f{}&&
                        project.GetDocument().FindObject(secondId)->transform.position==Vector2f{100,0},
                    "Individual pivot scaling must preserve each selected object's position");
    project.Undo();view.CyclePivotMode();view.SetTransformTool(TransformTool::Move);
    project.SetSelectedObject(id); view.Refresh(project,simulation,true);

    const Vector2f placedAt{180,100};const auto placeScreen=pipeframe::backend::sfml::WorldToScreen(context,{placedAt.x,placedAt.y});
    DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Right,placeScreen},context);
    DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Right,placeScreen},context);
    if(auto *placeAction=FindAction(view.GetContextMenu(),"PLACE OBJECT HERE")) {
        click(*placeAction);
    }
    else passed &= Check(false,"The viewport context menu must expose its placement action");
    passed &= Check(project.GetSelectedObject()&&
                        std::abs(project.GetSelectedObject()->transform.position.x-placedAt.x)<0.001f&&
                        std::abs(project.GetSelectedObject()->transform.position.y-placedAt.y)<0.001f,
                    "The viewport context action must create an object at its placement preview position");
    project.Undo();project.SetSelectedObject(id);view.Refresh(project,simulation,true);
    DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,point},context);
    DispatchInput(input,sf::Event::MouseMoved{point+sf::Vector2i{30,15}},context);
    DispatchInput(input,sf::Event::FocusLost{},context);
    passed &= Check(project.GetSelectedObject()->transform.position==pipeframe::Vector2f{},"Losing focus must cancel the live drag");
    DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,point},context);
    const auto overInspector=shell.Inspector().GetScreenPosition()+sf::Vector2f{20,20};
    DispatchInput(input,sf::Event::MouseMoved{sf::Vector2i(overInspector)},context);
    DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,sf::Vector2i(overInspector)},context);
    passed &= Check(project.GetSelectedObject()->transform.position!=pipeframe::Vector2f{} && project.CanUndo(),"World drag capture must complete over UI panels");
    project.Undo();
    simulation.Toggle(project.GetDocument().GetObjects());
    const auto ticks=project.GetRuntime().GetStatistics().visibleObjectCount;
    project.SynchronizeRuntime(); simulation.FixedUpdate(0.016f);
    passed &= Check(!simulation.IsPlaying() && project.GetRuntime().GetStatistics().visibleObjectCount==ticks,"Synchronizing a paused preview must not resume it");
    simulation.Toggle(project.GetDocument().GetObjects());
    DispatchInput(input,sf::Event::KeyPressed{sf::Keyboard::Key::Tab},context);
    passed &= Check(view.HasKeyboardFocus() && view.GetWorkspaceMode()==WorkbenchWorkspaceMode::Simulation,"Tab must focus UI without changing workspace mode");
    view.SetWorkspaceMode(WorkbenchWorkspaceMode::Zen);
    passed &= Check(shell.GetWorldBounds().size==sf::Vector2f{1100,800},"Zen must use the full viewport");
    click(shell.ExitZen()); passed &= Check(view.GetWorkspaceMode()==WorkbenchWorkspaceMode::Editor&&!shell.IsGameViewportSelected(),"Zen recovery must return to the editor viewport tab");
    project.GetDocument().SetProperty(id,"test.tool",true);
    project.SynchronizeRuntime();
    DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Right,point},context);
    DispatchInput(input,sf::Event::MouseMoved{sf::Vector2i(overInspector)},context);
    DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Right,sf::Vector2i(overInspector)},context);
    passed &= Check(!view.HasBlockingOverlay() && project.GetRuntime().GetStatistics().vertexCount==1 &&
                    project.GetRuntime().GetStatistics().geometryTimeMs==1 && !project.GetRuntime().HasWorldPointerCapture(),
                    "Project right-click tools must retain capture across Workbench panels");
    DispatchInput(input,sf::Event::KeyPressed{sf::Keyboard::Key::Tab},context);
    passed &= Check(project.GetRuntime().GetStatistics().spatialGridTimeMs==1,"Focused project UI must own Tab before Workbench traversal");
    project.GetDocument().RemoveProperty(id,"test.tool"); project.SynchronizeRuntime();
    simulation.Reset(project.GetDocument().GetObjects());
    project.SetSelectedObject(std::nullopt);
    passed &= Check(project.CreateTilemapAsset(),"Create editable map without external files");
    auto &mapEditor=project.GetTilemapEditor();
    const auto mapAssetId=mapEditor.AssetId();
    const auto paintPixel=context.MapWorldToPixel({15,15});
    input.HandleEvent({InputEventType::PointerPressed,PointerInput{PointerButton::Left,paintPixel}},context);
    auto brushSurface=std::make_shared<BrushPreviewSurface>();RenderContext brushContext(brushSurface);
    project.RenderTilemapEditing(brushContext);
    passed &= Check(mapEditor.HasPointerCapture() && brushSurface->footprint.size()==6 && brushSurface->ring.size()==192,
                    "Pencil cursor footprint remains rendered during left-button capture");
    const auto dragPixel=context.MapWorldToPixel({25,25});
    input.HandleEvent({InputEventType::PointerMoved,PointerMoveInput{dragPixel}},context);
    brushSurface->footprint.clear();project.RenderTilemapEditing(brushContext);
    passed &= Check(brushSurface->footprint.size()==6 && brushSurface->footprint.front().position==Vector2f{20,20},
                    "Captured pencil footprint follows the cursor during the stroke");
    project.HandleTilemapEvent({InputEventType::PointerReleased,PointerInput{PointerButton::Left,paintPixel}},context);
    passed &= Check(mapEditor.IsDirty()&&mapEditor.Document()->Tile(0,{1,1})==1,"Mapped viewport stroke changes authoring document");
    passed &= Check(project.Undo()&&mapEditor.Document()->Tile(0,{1,1})==0&&project.Redo(),"Editor routes tilemap undo/redo");
    passed &= Check(mapEditor.Save()&&!mapEditor.IsDirty(),"Painted map saves and reimports");
    mapEditor.Close();
    passed &= Check(project.BeginTilemapEditing(mapAssetId)&&mapEditor.Document()->Tile(0,{1,1})==1,"Paint survives editor reopen");
    project.RenderTilemapEditing(context);
    mapEditor.Close();
    const auto paintTarget=project.GetDocument().CreateObject("Paint target","test.object");
    project.GetDocument().AddComponent(paintTarget,{"pipeframe.tilemap2d",1,{{"asset",AssetReference{mapAssetId}}}});
    project.GetDocument().AddComponent(paintTarget,{"pipeframe.playground",1,
        {{"columns",std::int64_t{2}},{"rows",std::int64_t{2}},{"cellSize",10.0}}});
    project.SetSelectedObject(paintTarget);project.BeginTilemapEditing(mapAssetId);
    const auto outsidePaint=context.MapWorldToPixel({25,15});
    passed &= Check(!project.HandleTilemapEvent({InputEventType::PointerPressed,PointerInput{PointerButton::Left,outsidePaint}},context),
                    "Playground rejects stroke start outside its bounds");
    mapEditor.Configure(0,std::make_shared<TileBrush>(1),TilemapPaintShape::Fill);
    const auto insidePaint=context.MapWorldToPixel({5,5});
    project.HandleTilemapEvent({InputEventType::PointerPressed,PointerInput{PointerButton::Left,insidePaint}},context);
    project.HandleTilemapEvent({InputEventType::PointerReleased,PointerInput{PointerButton::Left,insidePaint}},context);
    passed &= Check(mapEditor.Document()->Tile(0,{0,0})==1&&mapEditor.Document()->Tile(0,{2,0})==0,
                    "Viewport fill respects smaller Playground on shared larger map");
    passed &= Check(mapEditor.Undo()&&!mapEditor.IsDirty(),"Bounded fill is one undo transaction");
    const auto sharedTarget=project.GetDocument().CreateObject("Shared map target","test.object");
    project.GetDocument().AddComponent(sharedTarget,{"pipeframe.tilemap2d",1,{{"asset",AssetReference{mapAssetId}}}});
    passed &= Check(project.CanMakeSelectedTilemapUnique()&&project.MakeSelectedTilemapUnique(),"Make selected map unique");
    const auto uniqueMapId=mapEditor.AssetId();
    const auto referenceFor=[&](SceneObjectId objectId){
        for(const auto &component:project.GetDocument().FindObject(objectId)->components)
            if(component.typeId=="pipeframe.tilemap2d")return std::get<AssetReference>(component.properties.at("asset")).assetId;
        return std::string{};
    };
    passed &= Check(uniqueMapId!=mapAssetId&&referenceFor(paintTarget)==uniqueMapId&&referenceFor(sharedTarget)==mapAssetId,
                    "Copy binds only selected object and preserves shared original");
    mapEditor.SetEnabled(false);
    passed &= Check(project.Undo()&&referenceFor(paintTarget)==mapAssetId&&project.Redo()&&referenceFor(paintTarget)==uniqueMapId,
                    "Make unique assignment supports scene undo and redo");
    project.BeginTilemapEditing(uniqueMapId);
    mapEditor.Configure(0,std::make_shared<TileBrush>(1));
    mapEditor.HandleEvent({InputEventType::PointerPressed,PointerInput{PointerButton::Left,{}}},GridCoordinate{0,0},true);
    project.GetDocument().RemoveObject(paintTarget);
    project.HandleTilemapEvent({InputEventType::PointerReleased,PointerInput{PointerButton::Left,paintPixel}},context);
    passed &= Check(!mapEditor.IsEnabled()&&!mapEditor.HasPointerCapture()&&!mapEditor.IsDirty(),"Deleting target cancels captured preview without committing");
    mapEditor.Close();
    {
        const auto camera=context.GetCamera();
        const auto target=project.GetDocument().CreateObject("Environment workflow","test.object");
        project.GetDocument().AddComponent(target,{TilemapComponentTypeId,1,{{"asset",AssetReference{mapAssetId}}}});
        project.GetDocument().AddComponent(target,{PlaygroundComponentTypeId,1,{{"columns",std::int64_t{8}},{"rows",std::int64_t{8}},{"cellSize",10.0}}});
        project.GetDocument().SetTransform(target,{{100,60},.6f,{1.5f,.75f}});project.SetSelectedObject(target);
        passed &= Check(project.BeginTilemapEditing(mapAssetId),"Begin transformed Playground editing");
        Transform2DComponent pose;pose.position={100,60};pose.rotation=.6f;pose.scale={1.5f,.75f};
        context.GetCamera().SetCenter({155,110});context.GetCamera().SetSize({500,350});
        for(float zoom:{.7f,1.7f}){
            context.GetCamera().SetZoom(zoom);
            const auto from=context.MapWorldToPixel(PlaygroundToWorld({35,35},pose)),to=context.MapWorldToPixel(PlaygroundToWorld({75,35},pose));
            mapEditor.Configure(0,std::make_shared<TileBrush>(1),TilemapPaintShape::Line);
            project.HandleTilemapEvent({InputEventType::PointerPressed,PointerInput{PointerButton::Left,from}},context);
            project.HandleTilemapEvent({InputEventType::PointerMoved,PointerMoveInput{to}},context);
            project.HandleTilemapEvent({InputEventType::PointerReleased,PointerInput{PointerButton::Left,to}},context);
            passed &= Check(mapEditor.Document()->Tile(0,{3,3})==1&&mapEditor.Document()->Tile(0,{7,3})==1&&mapEditor.Undo(),"Pan/zoom and rotated nonuniform object scale preserve gap-free cell strokes and undo");
        }
        project.SetSelectedObject(sharedTarget);project.HandleTilemapEvent({InputEventType::PointerMoved,PointerMoveInput{{10,10}}},context);
        passed &= Check(!mapEditor.IsEnabled(),"Changing Playground selection stops painting the previous target");
        project.SetSelectedObject(target);project.BeginTilemapEditing(mapAssetId);
        view.Refresh(project,simulation,false);shell.Assets().SelectCategory(assets::AssetType::Tilemap);
        const auto invoke=[&](const std::string &key){
            auto description=shell.Assets().DescribeView();
            std::function<bool(const pipeframe::ui::View &)> visit=[&](const auto &node){
                if(node.key==key&&node.onPressed&&node.enabled){node.onPressed();return true;}
                for(const auto &child:node.children)if(visit(child))return true;return false;
            };return visit(description);
        };
        passed &= Check(invoke("add")&&mapEditor.Document()->Layers().size()==2&&mapEditor.ActiveLayer()==1,"Declarative Add Layer action reaches engine document");
        passed &= Check(invoke("lock")&&mapEditor.Document()->Layers()[1].locked,"Declarative layer lock action");
        passed &= Check(invoke("map-undo")&&!mapEditor.Document()->Layers()[1].locked&&invoke("map-undo")&&mapEditor.Document()->Layers().size()==1,"Declarative undo restores lock and layer creation");
        const auto toolbarPoint=sf::Vector2i(shell.Toolbar().GetScreenPosition()+sf::Vector2f{8,8});
        DispatchInput(input,sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,toolbarPoint},context);
        DispatchInput(input,sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,toolbarPoint},context);
        passed &= Check(!mapEditor.HasPointerCapture()&&!mapEditor.IsDirty(),"UI hit regions cannot begin a map stroke");
        passed &= Check(invoke("test.density")&&mapEditor.ActiveBrush(),"Plugin brush selection is wired through declarative controls");
        auto brushView=shell.Assets().DescribeView();bool editedBrush=false;
        std::function<void(const pipeframe::ui::View &)> editSetting=[&](const auto &node){
            if(node.key=="density")for(const auto &child:node.children)if(child.onCommitted){child.onCommitted("0.8");editedBrush=true;}
            for(const auto &child:node.children)editSetting(child);
        };editSetting(brushView);
        passed &= Check(editedBrush&&std::get<double>(mapEditor.ActiveBrush()->Settings().at("density"))==.8,"Schema settings UI edits plugin brush");
        mapEditor.ConfigureGesture(0,1,TilemapPaintShape::Rectangle);
        const auto brushPoint=context.MapWorldToPixel(PlaygroundToWorld({35,35},pose));
        project.HandleTilemapEvent({InputEventType::PointerPressed,PointerInput{PointerButton::Left,brushPoint}},context);
        passed &= Check(mapEditor.HasPointerCapture(),"Custom brush starts on transformed target");
        project.GetDocument().RemoveObject(target);
        project.HandleTilemapEvent({InputEventType::PointerReleased,PointerInput{PointerButton::Left,brushPoint}},context);
        passed &= Check(!mapEditor.HasPointerCapture()&&mapEditor.Document()->DataLayer("test.density")->cells.At({3,3})==0,"Target deletion cancels custom brush without data writes");
        mapEditor.Close(true);context.GetCamera()=camera;
    }
    // Leave a representative running selection for deterministic UI snapshots.
    project.SetSelectedObject(id); view.Refresh(project,simulation,true);
    if(argc>=3 && std::string(argv[1])=="--snapshot") {
        sf::Vector2u size{argc>=4?static_cast<unsigned>(std::stoul(argv[3])):1100,800};
        std::size_t modeIndex=4;
        if(argc>=5&&std::ranges::all_of(std::string(argv[4]),[](unsigned char value){return std::isdigit(value);})){
            size.y=static_cast<unsigned>(std::stoul(argv[4]));modeIndex=5;
        }
        view.SetGridStep(0.0f);
        sf::RenderTexture snapshotTarget(size);auto snapshotContext = pipeframe::backend::sfml::MakeRenderContext(snapshotTarget);
        view.Layout(size,snapshotContext);
        const std::string mode=argc>static_cast<int>(modeIndex)?argv[modeIndex]:"";
        if(mode.starts_with("maps")) {
            project.SetSelectedObject(std::nullopt); project.BeginTilemapEditing(mapAssetId);
            if(mode=="maps-brush")project.GetTilemapEditor().SelectBrush("test.density");
            if(!view.IsAssetBrowserVisible())view.ToggleAssetsPanel();
            shell.Assets().SelectCategory(assets::AssetType::Tilemap);view.Refresh(project,simulation,false);
        }
        if(mode.starts_with("materials")||mode.starts_with("tilesets")){
            auto &assetEditor=project.GetVisualAssetEditor();
            passed &= Check(assetEditor.Create(project.GetAssetDatabase(),assets::AssetType::Material),"Create material in native editor fixture");
            const auto materialId=assetEditor.AssetId();
            passed &= Check(assetEditor.Set("texture",AssetReference{*textureAsset})&&assetEditor.Save(),"Bind imported texture in material editor");
            if(mode.starts_with("tilesets")){
                passed &= Check(assetEditor.Create(project.GetAssetDatabase(),assets::AssetType::Tileset),"Create tileset in editor fixture");
                passed &= Check(assetEditor.Set("material",AssetReference{materialId}),"Bind atlas material");
            }
            if(!view.IsAssetBrowserVisible())view.ToggleAssetsPanel();
            shell.Assets().SelectCategory(assetEditor.Type());view.Refresh(project,simulation,false);
            const auto *record=project.GetAssetDatabase().Find(assetEditor.AssetId());
            if(auto *row=FindAction(shell.Assets(),record->sourcePath.filename().string()+" | "+assets::ToString(record->type)+" | Ready"))click(*row);
        }
        if(mode=="zen") view.SetWorkspaceMode(WorkbenchWorkspaceMode::Zen);
        if(mode=="browser") view.ShowProjectBrowser({"/examples/Ant/project.pipeframe","/examples/SailBoat/project.pipeframe"});
        if(mode=="menu") view.ShowContextMenu({100,160});
        if(mode=="metrics") if(auto *button=FindAction(shell.Toolbar(),"METRICS")) click(*button);
        if(mode=="panels") if(auto *button=FindAction(shell.Toolbar(),"PANELS")) click(*button);
        if(mode=="floating") view.SetToolsFloating(true);
        view.Update(0.3f); view.Update(0.01f);
        if(mode.ends_with("-fields")||(mode=="maps-tools"||mode=="maps-brush")){
            snapshotContext.BeginScreen();view.RenderScreen(snapshotContext,simulation);view.Update(0);
            const auto point=shell.Assets().GetScreenPosition()+sf::Vector2f{5,40};
            view.HandleEvent(sf::Event::MouseWheelScrolled{sf::Mouse::Wheel::Vertical,(mode=="maps-tools"||mode=="maps-brush")?(mode=="maps-brush"&&size.x<1000?-12.f:-18.f):-100.f,sf::Vector2i(point)});
            view.Update(.05f);
        }
        snapshotTarget.clear(sf::Color(18,20,24));snapshotContext.BeginWorld();view.RenderWorldBackground(snapshotContext);
        project.GetRuntime().Render(snapshotContext);project.RenderTilemapEditing(snapshotContext);view.RenderWorldOverlay(snapshotContext);snapshotContext.BeginScreen();
        project.GetRuntime().RenderScreen(snapshotContext);
        view.RenderScreen(snapshotContext,simulation);
        snapshotTarget.display();
        if(!snapshotTarget.getTexture().copyToImage().saveToFile(argv[2])) return 1;
        project.GetTilemapEditor().Close(true);
        if(view.IsAssetBrowserVisible())view.ToggleAssetsPanel();
        view.Layout({1100,800},context);
    }
    const std::filesystem::path evidence=argc>2 && (std::string(argv[1])=="--r5-evidence"||std::string(argv[1])=="--portfolio-capture")?argv[2]:"";
    if(!evidence.empty()) {
        std::filesystem::create_directories(evidence);
        for(const sf::Vector2u size:{sf::Vector2u{1440,900},sf::Vector2u{800,700}}) {
            sf::RenderTexture target(size); auto capture = pipeframe::backend::sfml::MakeRenderContext(target);
            view.SetWorkspaceMode(WorkbenchWorkspaceMode::Editor);view.ShowWorkspace();view.Layout(size,capture);
            const auto save=[&](const std::string &name) {
                view.Update(.3f);view.Update(.01f);
                capture.BeginScreen();view.RenderScreen(capture,simulation);view.Update(0);
                target.clear(sf::Color{18,20,24});capture.BeginWorld();view.RenderWorldBackground(capture);
                project.GetRuntime().Render(capture);view.RenderWorldOverlay(capture);capture.BeginScreen();view.RenderScreen(capture,simulation);target.display();
                passed &= Check(target.getTexture().copyToImage().saveToFile(evidence/("editor-"+std::to_string(size.x)+"-"+name+".png")),"Save editor surface evidence");
            };
            view.Refresh(project,simulation,true);save("inspector");
            if(!view.IsAssetBrowserVisible())view.ToggleAssetsPanel();save("assets");view.ToggleAssetsPanel();
            shell.SetBottomDockVisible(true);view.Layout(size,capture);
            for(std::size_t tab=0;tab<shell.BottomTools().GetTabCount();++tab){shell.BottomTools().SetSelectedTab(tab);save("tab-"+std::to_string(tab));}
            shell.SetBottomDockVisible(false);view.Layout(size,capture);
            view.ShowContextMenu({100,160});save("menu");view.GetContextMenu().Dismiss();view.Update(.3f);
            if(auto *button=FindAction(shell.Toolbar(),"METRICS"))click(*button);save("metrics");
            if(auto *button=FindAction(shell.Toolbar(),"CLOSE"))click(*button);
            view.SetToolsFloating(true);save("floating");view.SetToolsFloating(false);shell.SetBottomDockVisible(false);view.Layout(size,capture);
            view.SetWorkspaceMode(WorkbenchWorkspaceMode::Zen);save("zen");view.SetWorkspaceMode(WorkbenchWorkspaceMode::Editor);
            view.ShowProjectBrowser({"/examples/Ant/project.pipeframe"});save("projects");view.ShowWorkspace();
            view.Layout(window.getSize(),context);
        }
        view.Layout(window.getSize(),context);
    }
    struct RuntimeProject { const char *path; const char *id; std::size_t extensions; std::size_t systems; };
    for(const auto &runtimeProject:std::array{
            RuntimeProject{PIPEFRAME_BASIC_PROJECT,"pipeframe.basic-simulation",2,2},
            RuntimeProject{PIPEFRAME_ANT_PROJECT,"pipeframe.ant-simulation",6,3},
#ifndef PIPEFRAME_ANT_REWORK_ONLY
            RuntimeProject{PIPEFRAME_SAILBOAT_PROJECT,"pipeframe.sailboat-simulation",5,4},
#endif
}){
        ProjectSession integrationProject(std::filesystem::temp_directory_path()/"pipeframe-runtime-integration");
        passed &= Check(integrationProject.OpenProject(runtimeProject.path,&error)&&integrationProject.GetRuntime().HasRuntime(),
                        "A migrated reference project must open through the standard runtime host");
        if(!integrationProject.GetRuntime().HasRuntime())continue;
        passed &= Check(integrationProject.GetRuntime().GetPluginDescriptor().id==runtimeProject.id&&
                            integrationProject.GetRuntime().GetExtensions().All().size()>=runtimeProject.extensions&&
                            integrationProject.GetRuntime().GetSystems().All().size()>=runtimeProject.systems,
                        "Reference projects must expose their extension and scheduled-system contracts");
        const std::vector<SceneObjectData> beforeReload(
            integrationProject.GetDocument().GetObjects().begin(),
            integrationProject.GetDocument().GetObjects().end());
        const auto componentTypeCount=integrationProject.GetRuntime().GetSceneComponentTypes().size();
        passed &= Check(integrationProject.GetRuntime().Reload(
                            integrationProject.GetDocument().GetObjects(),
                            integrationProject.GetSelectedObjectId(),&error)&&
                            integrationProject.GetRuntime().GetSceneComponentTypes().size()==componentTypeCount,
                        "Every reference runtime must hot reload with its component registry intact");
        passed &= Check(integrationProject.GetDocument().GetObjects().size()==beforeReload.size(),
                        "Reference runtime reload must preserve the authored scene object set");
        for(const auto &object:integrationProject.GetDocument().GetObjects()){
            const auto type=std::ranges::find(integrationProject.GetObjectTypes(),object.typeId,
                                              &SceneObjectTypeDescriptor::typeId);
            if(type==integrationProject.GetObjectTypes().end())continue;
            for(const auto &componentTypeId:type->componentTypeIds)
                passed &= Check(std::ranges::find(object.components,componentTypeId,
                                                  &SceneComponentData::typeId)!=object.components.end(),
                                "Reference project objects must migrate into their registered component schemas");
            const auto before=std::ranges::find(beforeReload,object.id,&SceneObjectData::id);
            passed &= Check(before!=beforeReload.end()&&before->components==object.components&&
                                before->prefabLinks==object.prefabLinks&&
                                before->properties==object.properties,
                            "Reference runtime reload must preserve components, references, and authored values");
        }
        SimulationSession integrationSimulation(integrationProject.GetRuntime());
        if(argc>1 && std::string(argv[1])=="--ant-scale-benchmark" && std::string(runtimeProject.id)=="pipeframe.ant-simulation") {
            sf::RenderTexture target({1440,900});auto capture = pipeframe::backend::sfml::MakeRenderContext(target);
            view.ShowWorkspace();view.SetWorkspaceMode(WorkbenchWorkspaceMode::Editor);
            view.Layout({1440,900},capture);
            std::string mapError;std::ifstream mapInput(integrationProject.GetProjectDirectory()/"Assets/Tilemaps/Main.pftilemap");
            const auto map=TilemapSerializer::Load(mapInput,mapError);if(!map){std::cerr<<mapError;return 1;}
            std::vector<Vector2f> positions{{96,108}};
            for(unsigned y=16;y+16<unsigned(map->Rows())&&positions.size()<5;y+=12)for(unsigned x=16;x+16<unsigned(map->Columns())&&positions.size()<5;x+=12){
                bool clear=true;for(int dy=-6;dy<=6;++dy)for(int dx=-6;dx<=6;++dx)if(map->IsSolid({int(x)+dx,int(y)+dy}))clear=false;
                for(auto position:positions)if(std::hypot(position.x-x,position.y-y)<65)clear=false;
                if(clear)positions.push_back({float(x),float(y)});
            }
            if(positions.size()!=5){std::cerr<<"Could not place five comparable colonies";return 1;}
            std::cout<<"Matched Main.pftilemap (authored collision placement), seed 1, 1000 configured ants/colony, 1x fixed 1/60s, 1440x900; 1200 warmup ticks, 180 measured frames\n";
            std::cout<<"Colony positions:";for(auto position:positions)std::cout<<" ("<<position.x<<","<<position.y<<")";std::cout<<'\n';
            bool performancePassed=true;
            int created=1;
            for(int colonyCount:{1,3,5}){
                integrationSimulation.Stop();
                for(;created<colonyCount;++created)if(!integrationProject.CreateObjectOfType("ant.colony",positions[created]))return 1;
                for(const auto &object:integrationProject.GetDocument().GetObjects())if(object.typeId=="ant.colony"){
                    integrationProject.SetSelectedObject(object.id);
                    integrationProject.SetSelectedComponentProperty("ant.colony","initialPopulation",std::int64_t{1000});
                    integrationProject.SetSelectedComponentProperty("ant.colony","randomSeed",std::int64_t{1});
                }
                integrationProject.SetSelectedObject(integrationProject.GetDocument().GetObjects().front().id);
                view.Refresh(integrationProject,integrationSimulation,true);view.FrameSelectionOrScene();view.Update(.1f);
                integrationSimulation.Toggle(integrationProject.GetDocument().GetObjects());
                for(int i=0;i<1200;++i)integrationSimulation.FixedUpdate(1.f/60.f);
                std::vector<double> samples;double stages[3]{};
                std::ofstream csv;if(argc>2){std::filesystem::create_directories(argv[2]);csv.open(std::filesystem::path(argv[2])/("colonies-"+std::to_string(colonyCount)+".csv"));csv<<"frame,simulation_ms,ui_ms,render_ms,total_ms\n";}
                for(int i=0;i<190;++i){
                    auto start=std::chrono::steady_clock::now();integrationSimulation.FixedUpdate(1.f/60.f);
                    auto simulated=std::chrono::steady_clock::now();view.Refresh(integrationProject,integrationSimulation,false);view.Update(1.f/60.f);
                    auto updated=std::chrono::steady_clock::now();target.clear();capture.BeginWorld();view.RenderWorldBackground(capture);
                    integrationProject.GetRuntime().Render(capture);view.RenderWorldOverlay(capture);capture.BeginScreen();
                    integrationProject.GetRuntime().RenderScreen(capture);view.RenderScreen(capture,integrationSimulation);target.display();
                    auto ended=std::chrono::steady_clock::now();
                    if(i>=10){auto ms=[](auto a,auto b){return std::chrono::duration<double,std::milli>(b-a).count();};
                        samples.push_back(ms(start,ended));stages[0]+=ms(start,simulated);stages[1]+=ms(simulated,updated);stages[2]+=ms(updated,ended);
                        if(csv)csv<<i-10<<','<<ms(start,simulated)<<','<<ms(simulated,updated)<<','<<ms(updated,ended)<<','<<samples.back()<<'\n';}
                }
                std::int64_t live=0;for(const auto &object:integrationProject.GetDocument().GetObjects())if(object.typeId=="ant.colony")
                    if(auto components=integrationProject.GetRuntime().InspectObjectComponents(object.id))for(const auto &component:*components)
                        if(component.typeId=="ant.colony-state")live+=std::get<std::int64_t>(component.properties.at("members"));
                std::sort(samples.begin(),samples.end());std::cout<<colonyCount<<" colonies; live ants "<<live<<"; median "<<samples[90]<<" ms; p95 "<<samples[171]<<" ms; mean simulation/UI/render ";
                for(auto stage:stages)std::cout<<stage/180<<" ";std::cout<<std::endl;
                performancePassed &= samples[171]<=16.67;
            }
            integrationSimulation.Stop();return !passed?1:performancePassed?0:2;
        }
        if(argc>1 && (std::string(argv[1])=="--ant-ui-benchmark" || std::string(argv[1])=="--ant-drag-benchmark") && std::string(runtimeProject.id)=="pipeframe.ant-simulation") {
            integrationProject.SetSelectedObject(integrationProject.GetDocument().GetObjects().front().id);
            sf::RenderTexture target({1440,900}); auto capture = pipeframe::backend::sfml::MakeRenderContext(target);
            view.ShowWorkspace(); view.SetWorkspaceMode(WorkbenchWorkspaceMode::Editor);
            view.Layout({1440,900},capture); view.Refresh(integrationProject,integrationSimulation,true);
            view.FrameSelectionOrScene(); view.Update(.1f);
            const bool dragBenchmark=std::string(argv[1])=="--ant-drag-benchmark";
            WorkbenchInput benchmarkInput(view,integrationProject,integrationSimulation);
            benchmarkInput.SetOnStateChanged([&](bool rebuild){view.Refresh(integrationProject,integrationSimulation,rebuild);});
            bool performancePassed=true;
            for(int mode=dragBenchmark?3:0;mode<(dragBenchmark?4:3);++mode) {
                if(mode==2) integrationSimulation.Toggle(integrationProject.GetDocument().GetObjects());
                std::vector<double> samples;
                double stages[5]{};
                std::ofstream csv;if(argc>2){std::filesystem::create_directories(argv[2]);csv.open(std::filesystem::path(argv[2])/("editor-"+std::to_string(mode)+".csv"));csv<<"frame,input_simulation_ms,ui_ms,world_ms,screen_ms,display_ms,total_ms\n";}
                for(int i=0;i<130;++i) {
                    const auto start=std::chrono::steady_clock::now();
                    if(mode==1) view.HandleEvent(sf::Event::MouseWheelScrolled{sf::Mouse::Wheel::Vertical,i%40<20?-1.f:1.f,{1380,750}});
                    if(mode==2) integrationSimulation.FixedUpdate(1.f/60.f);
                    if(mode==3){
                        const auto before=integrationProject.GetSelectedObject()->transform.position;
                        const auto pixel=capture.MapWorldToPixel(before);
                        DispatchInput(benchmarkInput,sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,{pixel.x,pixel.y}},capture);
                        DispatchInput(benchmarkInput,sf::Event::MouseMoved{{pixel.x+(i%2?8:-8),pixel.y}},capture);
                        DispatchInput(benchmarkInput,sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,{pixel.x+(i%2?8:-8),pixel.y}},capture);
                        if(integrationProject.GetSelectedObject()->transform.position==before){std::cerr<<"Drag fixture did not move the colony";return 1;}
                    }
                    const auto afterSimulation=std::chrono::steady_clock::now();
                    view.Refresh(integrationProject,integrationSimulation,false); view.Update(1.f/60.f);
                    const auto afterUI=std::chrono::steady_clock::now();
                    target.clear(); capture.BeginWorld(); view.RenderWorldBackground(capture);
                    integrationProject.GetRuntime().Render(capture); view.RenderWorldOverlay(capture);
                    const auto afterWorld=std::chrono::steady_clock::now();
                    capture.BeginScreen(); integrationProject.GetRuntime().RenderScreen(capture);
                    view.RenderScreen(capture,integrationSimulation);
                    const auto afterScreen=std::chrono::steady_clock::now();
                    target.display();
                    const auto ended=std::chrono::steady_clock::now();
                    if(i>=10) {
                        const auto ms=[](auto a,auto b){return std::chrono::duration<double,std::milli>(b-a).count();};
                        stages[0]+=ms(start,afterSimulation); stages[1]+=ms(afterSimulation,afterUI);
                        stages[2]+=ms(afterUI,afterWorld); stages[3]+=ms(afterWorld,afterScreen);
                        stages[4]+=ms(afterScreen,ended);
                        if(csv)csv<<i-10<<','<<ms(start,afterSimulation)<<','<<ms(afterSimulation,afterUI)<<','<<ms(afterUI,afterWorld)<<','<<ms(afterWorld,afterScreen)<<','<<ms(afterScreen,ended)<<','<<ms(start,ended)<<'\n';
                    }
                    if(i>=10) samples.push_back(std::chrono::duration<double,std::milli>(ended-start).count());
                }
                std::sort(samples.begin(),samples.end());
                std::cout<<"Ant 1440x900 offscreen "<<(mode==0?"paused selected":mode==1?"paused scrolling":mode==2?"playing one colony":"paused colony drag")
                    <<": median "<<samples[60]<<" ms, p95 "<<samples[114]<<" ms\n";
                std::cout<<"Mean stages ms (input/simulation, UI refresh/update, world submission, screen submission, offscreen display): ";
                for(auto stage:stages)std::cout<<stage/120<<" ";std::cout<<"\n";
                performancePassed &= samples[114]<=16.67;
            }
            view.Layout(window.getSize(),context);
            return !passed?1:performancePassed?0:2;
        }
        integrationSimulation.Toggle(integrationProject.GetDocument().GetObjects());
        integrationSimulation.FixedUpdate(1.0f/60.0f);
        integrationProject.GetRuntime().Render(context);
        integrationProject.GetRuntime().RenderScreen(context);
        if(!evidence.empty() && std::string(runtimeProject.id)=="pipeframe.ant-simulation") {
            if(!integrationProject.GetDocument().GetObjects().empty())integrationProject.SetSelectedObject(integrationProject.GetDocument().GetObjects().front().id);
            sf::RenderTexture target({1440,900});auto capture = pipeframe::backend::sfml::MakeRenderContext(target);
            view.ShowWorkspace();view.SetWorkspaceMode(WorkbenchWorkspaceMode::Editor);view.Layout({1440,900},capture);
            view.Refresh(integrationProject,integrationSimulation,true);view.FrameSelectionOrScene();view.Update(.1f);
            target.clear(sf::Color{18,20,24});capture.BeginWorld();view.RenderWorldBackground(capture);
            integrationProject.GetRuntime().Render(capture);view.RenderWorldOverlay(capture);capture.BeginScreen();
            integrationProject.GetRuntime().RenderScreen(capture);view.RenderScreen(capture,integrationSimulation);target.display();
            passed &= Check(target.getTexture().copyToImage().saveToFile(evidence/"ant-workbench.png"),"Save real Ant project in editor");
            for(int tick=0;tick<600;++tick)integrationSimulation.FixedUpdate(1.0f/60.0f);
            for(const auto &[label,options]:std::array<std::pair<const char*,WorldDebugOptions>,2>{{
                {"physics",{true,false}},{"mesh",{false,true}}}}){
                if(auto *button=FindAction(shell.Toolbar(),options.physics?"PHYSICS OFF":"MESH OFF"))click(*button);
                view.Update(.01f);
                // Ant point LOD has no triangles. Zoom into detailed geometry for mesh evidence.
                if(options.mesh){capture.SetCameraCenter({110,108});capture.SetCameraSize({40,25});capture.SetCameraZoom(1);}
                target.clear(sf::Color{18,20,24});capture.BeginWorld();view.RenderWorldBackground(capture);
                integrationProject.GetRuntime().Render(capture);
                integrationProject.GetRuntime().RenderDebug(capture,view.GetWorldDebugOptions());
                view.RenderWorldOverlay(capture);capture.BeginScreen();
                integrationProject.GetRuntime().RenderScreen(capture);view.RenderScreen(capture,integrationSimulation);target.display();
                passed &= Check(target.getTexture().copyToImage().saveToFile(evidence/(std::string("ant-debug-")+label+".png")),"Save Ant debug overlay evidence");
                if(auto *button=FindAction(shell.Toolbar(),options.physics?"PHYSICS ON":"MESH ON"))click(*button);
                view.Update(.01f);
            }
            if(std::string(argv[1])=="--portfolio-capture") {
                std::filesystem::create_directories(evidence/"editor-frames");
                std::filesystem::create_directories(evidence/"ant-frames");
                sf::RenderTexture antTarget({960,600});auto antCapture=pipeframe::backend::sfml::MakeRenderContext(antTarget);
                antCapture.SetCameraCenter({96,108});antCapture.SetCameraSize({145,90.625f});
                capture.SetCameraCenter({96,108});capture.SetCameraSize({170,118});capture.SetCameraZoom(1);
                for(int frame=0;frame<90;++frame) {
                    for(int step=0;step<4;++step)integrationSimulation.FixedUpdate(1.f/60.f);
                    if(frame==30)if(auto *button=FindAction(shell.Toolbar(),"PHYSICS OFF"))click(*button);
                    if(frame==60)if(auto *button=FindAction(shell.Toolbar(),"PHYSICS ON"))click(*button);
                    view.Refresh(integrationProject,integrationSimulation,false);view.Update(1.f/15.f);
                    target.clear(sf::Color{18,20,24});capture.BeginWorld();view.RenderWorldBackground(capture);
                    integrationProject.GetRuntime().Render(capture);integrationProject.GetRuntime().RenderDebug(capture,view.GetWorldDebugOptions());
                    view.RenderWorldOverlay(capture);capture.BeginScreen();integrationProject.GetRuntime().RenderScreen(capture);
                    view.RenderScreen(capture,integrationSimulation);target.display();
                    const auto file=std::to_string(1000+frame)+".png";
                    passed &= Check(target.getTexture().copyToImage().saveToFile(evidence/"editor-frames"/file),"Capture editor portfolio frame");
                    antTarget.clear(sf::Color{18,20,24});antCapture.BeginWorld();integrationProject.GetRuntime().Render(antCapture);antTarget.display();
                    passed &= Check(antTarget.getTexture().copyToImage().saveToFile(evidence/"ant-frames"/file),"Capture Ant portfolio frame");
                }
            }
            view.Layout(window.getSize(),context);
        }
        if(std::string(runtimeProject.id)=="pipeframe.ant-simulation") {
            integrationSimulation.Stop();
            const auto &antObjects=integrationProject.GetDocument().GetObjects();
            const auto ground=std::ranges::find(antObjects,std::string(PlaygroundEntityTypeId),&SceneObjectData::typeId);
            passed &= Check(ground!=antObjects.end(),"Ant ships an editor-authored Playground");
            if(ground!=antObjects.end()){
                integrationProject.SetSelectedObject(ground->id);
                const auto tile=std::ranges::find(ground->components,std::string(TilemapComponentTypeId),&SceneComponentData::typeId);
                passed &= Check(tile!=ground->components.end(),"Ant Playground exposes the shared Tilemap component");
                if(tile!=ground->components.end()){
                    const auto reference=std::get<AssetReference>(tile->properties.at("asset"));
                    passed &= Check(!reference.assetId.starts_with("source:"),"Shipped source reference migrates to stable project asset ID");
                    passed &= Check(integrationProject.BeginTilemapEditing(reference.assetId),"Ant map opens in shared viewport editor");
                    view.Refresh(integrationProject,integrationSimulation,true);view.FrameSelectionOrScene();
                    passed &= Check(std::abs(context.GetCameraCenter().x-192)<.01f&&std::abs(context.GetCameraCenter().y-108)<.01f,"Frame Playground uses its full bounds");
                    if(!evidence.empty()){
                        sf::RenderTexture target({1440,900});auto capture=pipeframe::backend::sfml::MakeRenderContext(target);
                        view.Layout({1440,900},capture);view.Refresh(integrationProject,integrationSimulation,true);view.FrameSelectionOrScene();view.Update(.3f);
                        target.clear();capture.BeginWorld();view.RenderWorldBackground(capture);integrationProject.GetRuntime().Render(capture);
                        view.RenderWorldOverlay(capture);capture.BeginScreen();integrationProject.GetRuntime().RenderScreen(capture);
                        view.RenderScreen(capture,integrationSimulation);target.display();
                        passed &= Check(target.getTexture().copyToImage().saveToFile(evidence/"ant-authored-playground.png"),"Save authored Ant Playground evidence");
                        view.Layout(window.getSize(),context);
                    }
                    integrationProject.GetTilemapEditor().Close(true);
                    auto invalidScene=antObjects;
                    for(auto &object:invalidScene)if(object.id==ground->id){object.transform.scale={2,2};SynchronizeTransformComponent(object);}
                    integrationProject.GetRuntime().SynchronizeScene(invalidScene);
                    passed &= Check(!integrationProject.GetRuntime().GetLastPluginError().empty(),"Unsupported Ant transform reports to editor without escaping UI");
                    integrationProject.GetRuntime().SynchronizeScene(antObjects);
                }
            }
            auto typedCallbacks=callbacks;
            typedCallbacks.createObjectOfType=[&](const auto &type,auto position) {
                passed &= Check(integrationProject.CreateObjectOfType(type,position),"Picker creates chosen registered entity");
            };
            const auto generatedRoot=assetRoot/"GeneratedUIProject";
            ProjectManager generatedManager(assetRoot/"generated-recent"); SceneDocument generatedScene;
            passed &= Check(generatedManager.CreateProject(generatedRoot,"Generated UI",generatedScene,&error),"Create source-authoring test project");
            typedCallbacks.generateModule=[&](const std::string &kind,const std::string &name) {
                if(kind!="Entity")return std::string("Unexpected kind");
                std::string failure;
                return ProjectScaffolder::AddModule(generatedRoot,GeneratedModuleKind::Entity,name,&failure)?std::string("Generated"):failure;
            };
            view.SetCallbacks(typedCallbacks);
            view.ShowWorkspace(); view.SetWorkspaceMode(WorkbenchWorkspaceMode::Editor);
            view.Refresh(integrationProject,integrationSimulation,true); view.Update(.1f);
            const auto before=integrationProject.GetDocument().GetObjects().size();
            auto *plus=FindAction(shell.Hierarchy(),"+");
            passed &= Check(plus!=nullptr,"Hierarchy plus exists for type picker"); if(plus)click(*plus);
            auto *beaconButton=FindAction(view.GetContextMenu(),"SIGNAL BEACON");
            passed &= Check(beaconButton && FindAction(view.GetContextMenu(),"FOOD SOURCE"),"Picker lists multiple registered project types");
            if(beaconButton)click(*beaconButton);
            passed &= Check(integrationProject.GetDocument().GetObjects().size()==before+1 &&
                integrationProject.GetSelectedObject()->typeId=="ant.signal-beacon","Plus creates beacon instead of default colony");
            view.Refresh(integrationProject,integrationSimulation,true); view.Update(.3f);
            if(!evidence.empty()) {
                sf::RenderTexture target({1440,900}); auto capture = pipeframe::backend::sfml::MakeRenderContext(target);
                view.Layout({1440,900},capture); view.Refresh(integrationProject,integrationSimulation,true);
                view.FrameSelectionOrScene(); view.Update(.1f);
                const auto saveBeacon=[&](const char *name) {
                    view.Update(.3f);target.clear(sf::Color{18,20,24});capture.BeginWorld();view.RenderWorldBackground(capture);
                    integrationProject.GetRuntime().Render(capture);view.RenderWorldOverlay(capture);capture.BeginScreen();
                    integrationProject.GetRuntime().RenderScreen(capture);view.RenderScreen(capture,integrationSimulation);target.display();
                    passed &= Check(target.getTexture().copyToImage().saveToFile(evidence/name),"Save Beacon editor evidence");
                };
                if(auto *fold=FindAction(shell.Inspector(),"v  COMPONENT  |  Transform"))click(*fold);
                saveBeacon("signal-beacon-inspector.png");
                view.ShowCreateObjectPicker();saveBeacon("create-object-picker.png");
                view.GetContextMenu().Dismiss();view.Update(.3f);view.Layout(window.getSize(),context);
            }
            view.ShowCreateObjectPicker();
            if(auto *cancel=FindAction(view.GetContextMenu(),"CANCEL"))click(*cancel);
            passed &= Check(integrationProject.GetDocument().GetObjects().size()==before+1,"Cancel does not create another entity");
            view.ShowCreateObjectPicker();view.Update(.3f);
            if(auto *source=FindAction(view.GetContextMenu(),"NEW ENTITY / COMPONENT / BEHAVIOUR"))click(*source);
            view.Update(.3f);
            auto *className=FindInput(view.GetContextMenu());passed &= Check(className!=nullptr,"Source dialog has class-name field");
            if(className) {
                click(*className);
                for(const char c:std::string("InspectorProbeEntity"))view.HandleEvent(sf::Event::TextEntered{static_cast<char32_t>(c)});
                view.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Enter});
            }
            if(!evidence.empty()) {
                sf::RenderTexture target(window.getSize());auto capture = pipeframe::backend::sfml::MakeRenderContext(target);
                view.Layout(window.getSize(),capture);view.Update(.3f);target.clear(sf::Color{18,20,24});
                capture.BeginScreen();view.RenderScreen(capture,integrationSimulation);target.display();
                passed &= Check(target.getTexture().copyToImage().saveToFile(evidence/"source-generation.png"),"Save source creation form evidence");
                view.Layout(window.getSize(),context);
            }
            if(auto *generate=FindAction(view.GetContextMenu(),"GENERATE"))click(*generate);
            passed &= Check(std::filesystem::exists(generatedRoot/"Source/Entities/InspectorProbeEntity.h"),"Editor dialog generates actual entity source");
            view.GetContextMenu().Dismiss();view.Update(.3f);
            view.SetCallbacks(callbacks);
        }
        integrationSimulation.Stop();
    }
    if(passed) std::cout<<"All Workbench UI acceptance checks passed.\n";
    std::filesystem::remove_all(assetRoot); std::filesystem::remove(assetSource);
    std::filesystem::remove(workspaceLayoutPath);
    return passed?0:1;
}
