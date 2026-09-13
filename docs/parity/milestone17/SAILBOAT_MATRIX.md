# SailBoat forensic parity matrix

Reference roots: `SailBoatPezza/src`, `SailBoatPezza/res`, its README, the
SailBoat video at the timestamp in [the reference index](references/README.md),
and `examples/SailBoatSimulation`. Every row is classified.

| Area | Reference contract and evidence | Current classification | PipeFrame evidence / observed gap | Owner |
|---|---|---|---|---|
| Polar lookup | Wind-relative angle selects/interpolates reference boat speed [S] | matched | All source anchor/interpolation cases and knot conversion have numerical fixtures [P] | 17H complete |
| Apparent wind | Wind heading/speed are transformed into boat-local inputs [S,V] | matched | Boat updater and four-input fixtures verify incoming-wind signs [P] | 17H complete |
| Steering | One neural output drives bounded angular command at configured speed [S] | matched | Rudder clamp and exact degree/radian integration fixtures [P] | 17H complete |
| Integration | Position, heading, speed and trajectory advance from scaled `dt` [S] | matched | Deterministic update/reset replay and bounded-history fixtures [P] | 17H complete |
| Crash bounds | Leaving the environment ends/devalues an agent [S] | matched | Boundary fixture verifies clamp, zero speed, and inactive crash state [P] | 17H complete |
| Start | Directed start sets spawn point and initial heading [S,V] | matched | Race course/editor phase and agent reset fixtures [P] | 17H complete |
| Marks | Ordered marks target Pezzza's first endpoint; the direction segment remains editor/render metadata [S,V] | matched | Differential fixture rejects proximity to the segment away from its endpoint [P] | 17H complete |
| Finish | Finish endpoint completes only after ordered marks [S,V] | matched | Ordered waypoint/finish fixture covers task and boat completion [P] | 17H complete |
| Score | Endpoint distance, target index, target time, and completion bonus rank agents [S] | matched | Exact one-step source-formula assertion plus lifecycle history checks [P] | 17H complete |
| Reset/generalization | New trials reset state and evaluate a policy on varied courses/seeds [S,V] | matched | Public generalization evaluation returns per-trial/aggregate results across different course sizes, starts, and winds; exploration reseeding is deterministic and distinct [P] | 17H complete |
| Network inputs | Four live inputs describe target/wind/race state [S] | matched | Endpoint and wind values are asserted numerically in boat space [P] | 17F/17H complete |
| Network output | One activated value controls rudder/turn command [S] | matched | Tanh output, clamp, and angular integration are asserted [P] | 17F/17H complete |
| DAG/network | Feed-forward topology validates order and executes activations [S] | matched | `PipeFrame::Learning` DAG/network tests [P] | 17F complete |
| Mutation | Node, connection, bias, weight and value mutations use configured probabilities [S] | matched | PipeFrame deterministic mutator tests and post-neutral-generation topology checks [P] | 17F/17H complete |
| Selection/elites | Score-weighted selection retains elites and mutates duplicate signatures before refill [S] | matched | Selector and generation lifecycle tests [P] | 17F/17H complete |
| Iteration rollover | All agents evaluate, rank, evolve, reset, and increment iteration [S] | matched | Neutral generation-zero and complete rollover/history fixtures [P] | 17F/17H complete |
| Exploration restart | R/new exploration changes seed and escapes stalled searches [S] | matched | Runtime action plus distinct deterministic post-rollover genome assertion [P] | 17H complete |
| Training timer | Current iteration time and limit remain visible while running [S,V] | matched | Async/synchronous clock assertions and persistent top metric outside all tabs [P,R] | 17H complete; 17I styling |
| Async training | Background training completes without blocking presentation [S] | matched | Timer advances during background work and normal generation/history output is published [P] | 17F/17H complete |
| History/results | Best score, progress, iterations and exploration results persist in UI [S,V] | matched | Compact training-result card stays visible and the independent history drawer retains score, completion, generation table, and detail views [P,R] | 17I complete |
| Genome save | Best genome is saved on configured periods [S] | matched | Versioned PipeFrame genome and trainer persistence tests [P] | 17F complete |
| Resume | Checkpoint restores genome, trainer metadata, RNG and progression deterministically [S] | matched | Fixed-seed continuation compares every evolved genome after resume; async mode changes persist [P] | 17F/17H complete |
| Course save/load | Single race file saves and restores start, marks and finish [S] | matched | `.pfrace` round trip and integrated dashboard save/load assertions [P] | 17H complete |
| Editor clear | Clear removes current marks/race editing state [S] | matched | Editor scene-edit tests [P] | 17H complete |
| Start editing | Two right clicks set position then direction [S] | matched | Editor phase tests [P] | 17H complete |
| Mark editing | Two right clicks add ordered position/direction marks [S] | matched | Editor phase tests [P] | 17H complete |
| Finish editing | Two right clicks define segment endpoints [S] | matched | Editor phase tests [P] | 17H complete |
| Water | Animated tiled water and height/effect passes fill world [S,V] | matched | Pezzza-density ping-pong height field, normal/refraction shader, tiled background, wake sources, fallback, and renderer/performance fixtures [P] | 17I complete |
| Seascape/depth | Parallax sea texture and depth affect boat presentation [S,V] | matched | Original seascape/depth resources are PipeFrame-owned and applied by the renderer while compact HUD surfaces preserve the water view [P,R] | 17I complete |
| Boats/ghosts | Best boat, population ghosts, orientation and depth are visible [S,V] | matched | Batched textured boats, best/ghost distinction, orientation, depth inputs, best-only control, and performance fixtures [P] | 17I complete |
| Best-only | B toggles population ghosts while keeping best boat [S] | matched | Runtime shortcut, dashboard action, and rendering assertions [P] | 17H complete |
| Trajectory | Leading/selected course history is drawn and bounded [S,V] | matched | PipeFrame path commands and renderer/performance tests [P] | 17E complete |
| Target guidance | Arrow/labels expose the next mark and direction [S,V] | matched | Dotted route, arrow, target number, and endpoint source share the live task target [P] | 17H complete; 17I styling |
| Marks | Start green, ordered yellow marks, and finish red retain readable labels [S,V] | matched | Reference semantic colors, ordered labels, dotted guidance, target pulse, and endpoint-consistent geometry are retained [P] | 17I complete |
| Wind HUD | Direction and speed remain readable over water [S,V] | matched | Dashboard fixture and runtime state [P,R] | 17I verify |
| Selected boat | Compact card shows live race/boat state [S,V] | matched | Independent right card exposes identity, status, speed, distance, score, and target in the reference stack [P,R] | 17I complete |
| Live network | Independent graph shows live node values, labels, signed weighted edges and topology [S,V] | matched | Wide shallow lower-right drawer renders named inputs/output, values, signed colors, weighted directed edges, and evolved topology [P,R] | 17I complete |
| Independent drawers | Editor, settings, timer, result, boat, network, and wind occupy compact edge surfaces [S,V] | matched | Eight independent workflow drawers plus persistent wind/iteration cards replace the full-height tab surface [P,R] | 17I complete |
| Bottom transport | Start/pause and full-speed controls remain centered over world [S,V] | matched | Compact bottom play/pause surface composes with the host speed control and remains reachable at every tested size [P,R] | 17I complete |
| Follow best | F follows best boat; Tab resets reference camera [S] | intentional difference | Follow is asserted through live playback; Tab remains reserved by the host and camera reset needs final help text [P] | 17H complete; 17J document |
| Pause/full speed | Space pauses and S toggles full-speed reference mode [S] | intentional difference | PipeFrame reserves P for pause; S is retained [P] | 17J document |
| UI/world toggles | U hides UI; D hides world rendering [S] | matched | Runtime shortcuts and configuration propagation exist [P] | 17H complete |
| Sound | Mark advancement plays `bubble_2.wav` at 30% default volume [S] | matched | PipeFrame audio handle/cache tests and runtime implementation [P] | 17E complete |
| Performance | 1k default and 10k stress population stay responsive with bounded history [S,V] | matched | 1k frame budget and 10k × 60-update bounded-history regression [P] | 17H complete |
| Demo build | `DEMO=1` provides a presentation-focused target [S] | matched | Runtime/Zen presentation uses the same compact reference composition and releases editor chrome/input [P,R] | 17I complete |
| Hover/pressed/selected | Cards, handles and controls animate clear states [V] | matched | Hover lift, pressed/selected colors, focus, reduced motion, and eased drawer end states pass framework/interaction regressions [P] | 17I complete |
| SFML boundary | Domain/training code should consume PipeFrame-neutral APIs [P] | incorrect | Training, runtime ABI, audio, and graphics-resource ownership are neutral; final example draw/configuration adapters are a 17J removal gate [P] | 17J |

Source/video version note: source defaults to 2,000 agents while the supplied
`res/conf.txt` overrides population to 1,000, matching the documented/video
working configuration. The matrix preserves both values. PipeFrame's `P` pause
and workspace use of Tab are the only currently accepted host-level key changes;
their help text and replacement camera-reset action must still be verified in
17I/17J.
