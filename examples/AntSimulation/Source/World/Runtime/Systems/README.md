# Systems

PipeFrame's registered phases execute colony lifecycle, movement, foraging,
cleanup, live telemetry and rendering. AntMovementSystem, AntForagingSystem and
AntCleanupSystem implement FixedUpdateSystem and operate on engine scene data.
ColonySpawnerBehaviour handles each colony's attached fixed-update lifecycle.

AntMovementSystem uses shared engine steering, body integration and storage.
AntPoseAlgorithms and AntLegPose implement the domain pose calculations.
WorkerBehavior and MarkerSampler are domain rule helpers called by the foraging
system; they do not own a population or schedule another update loop.

Headless AntWorld updates invoke these same systems. The obsolete
AntSimulationPipeline and separate aggregate population owner are removed.
