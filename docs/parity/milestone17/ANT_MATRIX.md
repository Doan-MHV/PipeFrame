# Ant forensic parity matrix

Reference roots: `AntPezzaSource/src`, `AntPezzaSource/res`, the Ant video at the
timestamps in [the reference index](references/README.md), and the current
`examples/AntSimulation` project. Every row is classified.

| Area | Reference contract and evidence | Current classification | PipeFrame evidence / observed gap | Owner |
|---|---|---|---|---|
| Roles | Followers sample 64 times; explorers sample 8 over a narrower FOV; a 10% explorer share is enough to improve adaptation (video 0:00–0:35; `configuration.hpp`, `marker_sampler.hpp`) [S,V] | matched | Worker behavior and sampler tests cover both roles [P] | 17G verify |
| Objective selection | Food, colony, and matching marker objectives compete by sampled score [S] | matched | `MarkerSampler`, `WorkerBehavior` and tests [P] | 17G verify |
| Exhaustion fallback | Below the refill threshold, an outbound ant returns only when it is off a marker trail, and uses follower sampling while resupplying (video 4:30–5:19; `marker_sampler.hpp`, `ant_updater.hpp`) [S,V] | matched | Behavior fixture exists [P] | 17G verify |
| Trail timeout exception | Stale trail following clears focus after the reference timeout rule [S] | matched | Ant state/updater implementation [P] | 17G verify |
| Marker deposition | Drops depend on distance, current state, walk time, and maximum intensity [S] | matched | Pheromone/worker tests [P] | 17G verify |
| Marker decay | Intensity decays by configured rate and disappears below threshold [S] | matched | `PheromoneField` tests [P] | 17D/17G |
| Marker sampling degradation | Cell sampling coefficient reduces repeated attraction [S] | matched | Environment and sampler implementation [P] | 17G verify |
| Colony ownership | Markers carry colony identity and ants ignore incompatible ownership (video 5:37–7:29) [S,V] | matched | Marker ownership tests [P] | 17G verify |
| Foreign-marker erosion | A foreign deposit reduces the resident marker; ownership changes only after its intensity reaches zero (video 7:30–7:40; `world_cell.hpp`) [S,V] | matched | `AntBehaviorParityTests` fixes the 4→3→-1 resident sequence and takeover on the following deposit [P] | 17G complete |
| Food pickup | Ant takes cell food, switches state, and carries one unit [S] | matched | Food/worker tests [P] | 17G verify |
| Colony delivery | Returning carrier credits colony food and resets state [S] | matched | Colony/worker tests [P] | 17G verify |
| Energy | Movement consumes energy; food or colony contact refills it (video 3:15–3:39) [S,V] | matched | Ant updater tests [P] | 17G verify |
| Death cleanup | Exhausted/dead ants leave population and physics in the same update (video 3:40–4:29; `ant_updater.hpp`) [S,V] | matched | Common `PhysicsWorld2D` and Ant synchronization tests verify immediate body removal [P] | 17D complete; 17G parity |
| Births | Delivered food enters a colony resource pool and pays for new ants using the configured explorer probability (video 5:21–5:32; `colony_updater.hpp`) [S,V] | matched | Cumulative births and per-colony reserve/population are fixed at parity checkpoints; four colonies create 48 funded ants symmetrically [P] | 17G complete |
| Encounters | Same/foreign colony contacts apply the reference encounter policy [S] | matched | Contact regression proves friendly exclusion, reciprocal foreign alerts, dead exclusion, and Pezzza's intentionally empty soldier handler [P] | 17G complete |
| Collision broadphase | Uniform cell grid supplies nearby bodies [S] | matched | Ant contact resolution uses PipeFrame body primitives, uniform-index broadphase, and common mass-weighted circle solver [P] | 17D complete |
| Avoidance | Same-colony nearby ants steer to avoid overlap [S] | matched | Avoidance system tests [P] | 17G verify |
| Walls and raycasts | Grid walls block motion and marker sampling [S] | matched | Ant wall policy uses PipeFrame grid DDA and common movement rollback/bounds primitives; marker-sampler and laboratory tests pass [P] | 17D complete |
| Map import | Image pixels create walls, food, and initial world cells [S] | matched | `WorldMapLoader` tests and original assets [P] | 17G verify |
| Dynamic colonies | User can add arbitrary colonies with position/color/initial population [S,V] | matched | Editor tool and colony tests [P] | 17G verify |
| Colony removal | A colony and its dependent state can be removed safely [S] | matched | World removal clears the colony's ants, physics bodies, and transient/persistent markers without disturbing competitors [P] | 17G complete |
| Multi-colony symmetry | Two or more colonies compete under identical rules [S,V] | matched | Scripted four-colony run proves equal funded population/reserve accounting and stable independent ownership [P] | 17G complete |
| Parallel update | Threaded update supports large populations [S] | matched | Independent timer/energy advancement uses PipeFrame `ThreadPool`; shared physics, behavior, and RNG remain ordered; stress tests cover throughput [P] | 17C/17G complete |
| Determinism | Repeatable single-thread run and race-free parallel RNG; Pezzza disables multithreading for deterministic recordings (video 10:59–11:13) [S,V] | matched | Fixed-tick full-state signatures match same-seed replay and one/four-worker execution exactly; worker count is editor-exposed [P] | 17G complete |
| Ant body rendering | Near view uses articulated body/legs and far view uses sprite/scale boost [S,V] | matched | Geometry/renderer tests and PipeFrame-owned original textures [P] | 17E complete |
| Dynamic colors | State and colony colors convey role/objective [S,V] | matched | Renderer/settings implementation [P] | 17G verify |
| Markers/food/colonies | World overlays communicate intensity, ownership, quantity, and colony state [S,V] | matched | Source assets, dynamic colony colors, intensity controls, renderer fixtures, and retained 17I HUD comparisons preserve the world as the dominant visual surface [P,R] | 17I complete |
| Shadows | Asynchronous wall/ant shadow geometry and soft cards [S,V] | matched | PipeFrame-owned shader/texture lifetime, asynchronous geometry tests, smoked glass surfaces, and visual review [P,R] | 17I complete |
| Debug views | Physics, walls, sampling, paths, markers, and entity detail toggles [S] | matched | Simulation Settings exposes grid, markers, shadows, targets, physics, ants, dynamic colors, intensity, and worker mode; runtime regression verifies propagation [P] | 17G complete; 17I appearance |
| Selection | Click selects an ant/colony and highlights it in world [S,V] | matched | Inspector selection tests [P] | 17G verify |
| Follow | Selected ant can drive camera follow and be released [S,V] | matched | Inspector/runtime action [P] | 17G verify |
| Selected-ant card | Preview, name/speed, blocked, energy, distance, food, visibility and follow controls [V] | matched | Compact right drawer exposes preview, identity/state, three actions, live fields, and energy/speed/blocked bars; selection opens it on the availability transition [P,R] | 17I complete |
| Colony cards | Per-colony carousel, color, population, food, flow, and history [S,V] | matched | Compact right colony drawer provides a selectable colony list, accented summary, and population/collection history without occupying the world [P,R] | 17I complete |
| Timer/profiler | Always-readable simulation timer at top and profiler surface [S,V] | matched | Persistent top timer and independent left profiler drawer are verified across the resolution board [P,R] | 17I complete |
| Transport/modes | Bottom play/pause/speed and simulation/editor/Zen transitions [S,V] | matched | Centered bottom runtime transport composes with Workbench speed/mode controls; Zen releases UI input [P,R] | 17I complete |
| Editor | Brushes edit walls, food, colonies, and clear/commit state [S] | matched | Editor tool tests [P] | 17G verify |
| Independent drawers | Small top/bottom/left/right handles preserve the visible world [S,V] | matched | Six independently anchored drawers, compact rotated handles, overlap-aware opening, and 640–2560 reachability assertions [P,R] | 17I complete |
| Hover/pressed/selected | Cards and controls animate distinct interaction states [V] | matched | Hover lift, pressed/selected colors, focus, reduced motion, and eased drawer end states pass framework/interaction regressions [P] | 17I complete |
| Zen/demo state | UI can recede while simulation remains legible [S,V] | matched | Compact composition preserves the world and Zen hides/relinquishes every dashboard surface [P,R] | 17I complete |
| Shortcuts | Escape exits active edit/selection; R restarts where exposed [S] | matched | Runtime/editor handling exists [P] | 17G verify |
| Persistence | Configuration/map assets reproduce a scenario [S] | matched | Map/configuration plus stored seed reproduce the same full-state signature at fixed ticks; checkpoint records expose colony, marker, birth/death, food, and physics totals [P] | 17G complete |
| Audio | No Ant-specific sound contract was found in supplied source/video [S,V] | intentional difference | PipeFrame Ant has no required project audio [P] | 17J document |
| SFML boundary | Domain code should consume PipeFrame-neutral types | incorrect | Spatial, physics, contact, raycast, field, image, and resource ownership are neutral and protected by lint; final example draw/configuration adapters are a 17J removal gate [P] | 17J |

Source/video version note: the source includes current and `*_old` simulation
paths. Only the current `application.hpp`/`simulation.hpp` path defines logic
parity; old-path shortcuts are historical evidence. The video defines the target
drawer composition when source UI variants overlap.
