#include <PipeFrame/Core/AsyncTask.h>
#include <PipeFrame/Core/DeterministicRandom.h>
#include <PipeFrame/Core/FixedStepScheduler.h>
#include <PipeFrame/Core/Profiler.h>
#include <PipeFrame/Core/Signal.h>
#include <PipeFrame/Core/ThreadPool.h>
#include <PipeFrame/Data/GenerationalStore.h>
#include <PipeFrame/Data/Grid2D.h>
#include <PipeFrame/Spatial/GridRaycast.h>
#include <PipeFrame/Spatial/UniformSpatialIndex.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <vector>

namespace {
void TestGrid() {
    pipeframe::Grid2D<int> grid(3,2,4);
    assert(grid.Size()==6 && grid.At({2,1})==4);
    grid.At({0,0})=9;
    assert(grid.TryGet({0,0}) && *grid.TryGet({0,0})==9);
    assert(!grid.TryGet({3,0}));
    assert(grid.Clamp({-2,8})==pipeframe::GridCoordinate({0,1}));
}

void TestSpatialIndexAgainstBruteForce() {
    using Index=pipeframe::UniformSpatialIndex<std::uint32_t>;
    Index index;
    index.Initialize({{-10.0f,-5.0f},{20.0f,10.0f}},2.0f);
    std::vector<Index::Entry> entries{{1,{-10,-5}},{2,{-9,-4}},{3,{0,0}},{4,{9.999f,4.999f}},{5,{10,5}}};
    index.Rebuild(entries);
    assert(index.Size()==5 && index.CoordinateOf({10,5})==pipeframe::GridCoordinate({9,4}));
    for (const auto center : std::vector<pipeframe::Vector2f>{{-10,-5},{0,0},{9,5},{-2,1}}) {
        constexpr float radius=3.25f;
        auto actual=index.QueryRadius(center,radius);
        std::vector<std::uint32_t> expected;
        for(const auto &entry:entries) if(pipeframe::LengthSquared(entry.position-center)<=radius*radius) expected.push_back(entry.id);
        std::sort(actual.begin(),actual.end()); std::sort(expected.begin(),expected.end()); assert(actual==expected);
    }
    assert(index.Update(1,{0.5f,0.5f}));
    assert(index.Remove(2) && !index.Remove(2));
    assert(index.Insert(8,{-9.5f,4.5f}) && !index.Insert(8,{0,0}));
    auto nearby=index.QueryRadius({0,0},1.0f);
    assert(std::find(nearby.begin(),nearby.end(),1)!=nearby.end());
    assert(index.Size()==5);
}

void TestRaycast() {
    const auto hit=pipeframe::RaycastGrid({0.5f,1.5f},{1,0},10,{0,0},1,5,3,
        [](pipeframe::GridCoordinate cell){ return cell==pipeframe::GridCoordinate{3,1}; });
    assert(hit && hit->cell==pipeframe::GridCoordinate({3,1}));
    assert(std::abs(hit->distance-2.5f)<0.0001f && hit->normal==pipeframe::Vector2i({-1,0}));
}

void TestCoreServices() {
    pipeframe::GenerationalStore<int> store;
    const auto first=store.Emplace(7); const auto kept=store.Emplace(8);
    assert(*store.Get(first)==7); assert(store.Remove(first) && !store.Get(first));
    assert(store.Values().size()==1 && store.Values().front()==8 && *store.Get(kept)==8);
    const auto second=store.Emplace(11); assert(second.index==first.index && second.generation!=first.generation);

    pipeframe::DeterministicRandom a(42),b(42);
    for(int i=0;i<20;++i) assert(a.NextU64()==b.NextU64());
    auto streamA=a.Stream(7),streamB=a.Stream(7); assert(streamA.NextU64()==streamB.NextU64());

    pipeframe::Signal<int> signal; int total=0;
    const auto one=signal.Connect([&](int value){total+=value;});
    signal.Connect([&](int value){total+=value*2;}); signal.Emit(3); assert(total==9);
    assert(signal.Disconnect(one)); signal.Emit(1); assert(total==11);

    pipeframe::FixedStepScheduler scheduler(0.1,3); int steps=0;
    assert(scheduler.Advance(0.35,[&](double delta){assert(delta==0.1);++steps;})==3 && steps==3);
    assert(scheduler.Alpha()<1.0);

    pipeframe::ThreadPool pool(2);
    auto firstTask=pool.Submit([]{return 20;}); auto secondTask=pool.Submit([]{return 22;});
    assert(firstTask.get()+secondTask.get()==42);
    auto async=pipeframe::RunAsync([]{return 17;}); assert(async.get()==17);

    pipeframe::Profiler profiler; profiler.Record("update",1.5); { pipeframe::ProfileScope scope(profiler,"update"); }
    const auto sample=profiler.Get("update"); assert(sample.count==2 && sample.totalMilliseconds>=1.5);
}
}

int main() { TestGrid(); TestSpatialIndexAgainstBruteForce(); TestRaycast(); TestCoreServices(); }
