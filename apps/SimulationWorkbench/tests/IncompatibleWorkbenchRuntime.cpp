#include <PipeFrame/Project/ProjectRuntime.h>

class IncompatibleRuntime final : public pipeframe::ProjectRuntime {
public:
    const char *GetName() const override{return "Rejected runtime";}
    pipeframe::ProjectPluginDescriptor GetPluginDescriptor()const override{auto descriptor=pipeframe::ProjectPluginDescriptor{"test.workbench-runtime","Rejected runtime"};
#ifdef PIPEFRAME_TEST_OLD_ABI
        descriptor.abiVersion=pipeframe::ProjectPluginAbiVersion-1;
#endif
        return descriptor;}
    bool RegisterPlugin(pipeframe::PluginRegistrar &registrar,std::string &error)override{
        registrar.Action({"rejected.action",{},"Rejected Action","Test",{},"",[](const auto &){}},&error);
        error="Intentional registration failure.";return false;
    }
    bool Load(const pipeframe::ProjectRuntimeContext &,std::string &)override{return true;}
    std::span<const pipeframe::SceneObjectTypeDescriptor> GetSceneObjectTypes()const override{return {};}
    void SynchronizeScene(std::span<const pipeframe::SceneObjectData>)override{}
    void SetSelectedObject(std::optional<pipeframe::SceneObjectId>)override{}
    std::optional<pipeframe::SceneObjectId> HitTest(pipeframe::Vector2f)const override{return {};}
    void Start()override{} void FixedUpdate(float)override{} void Render(RenderContext &)override{}
    void Reset()override{} void Stop()override{} void Unload()override{}
};

extern "C" PIPEFRAME_PROJECT_RUNTIME_EXPORT pipeframe::ProjectRuntime *PipeFrameCreateProjectRuntime(){return new IncompatibleRuntime;}
extern "C" PIPEFRAME_PROJECT_RUNTIME_EXPORT void PipeFrameDestroyProjectRuntime(pipeframe::ProjectRuntime *runtime){delete runtime;}
