#ifndef SAILBOAT_SIMULATION_TYPES_H
#define SAILBOAT_SIMULATION_TYPES_H

namespace sailboat_simulation {

inline constexpr const char *RaceStartTypeId = "sailboat.race-start";
inline constexpr const char *FinishLineTypeId = "sailboat.finish-line";
inline constexpr const char *WaypointTypeId = "sailboat.waypoint";
inline constexpr const char *EnvironmentTypeId = "sailboat.environment";
inline constexpr const char *TrainingSettingsTypeId = "sailboat.training-settings";

inline constexpr const char *LineLengthKey = "lineLength";
inline constexpr const char *WaypointRadiusKey = "waypointRadius";
inline constexpr const char *WaypointOrderKey = "waypointOrder";

inline constexpr const char *WorldSizeKey = "worldSize";
inline constexpr const char *WindDirectionKey = "windDirectionDegrees";
inline constexpr const char *WindSpeedKey = "windSpeed";
inline constexpr const char *WaterAnimationKey = "waterAnimation";
inline constexpr const char *DrawBestOnlyKey = "drawBestOnly";
inline constexpr const char *DrawGhostBoatsKey = "drawGhostBoats";
inline constexpr const char *HighlightBestKey = "highlightBest";
inline constexpr const char *DrawBestTrajectoryKey = "drawBestTrajectory";
inline constexpr const char *DrawWaypointLabelsKey = "drawWaypointLabels";
inline constexpr const char *ShowTargetGuideKey = "showTargetGuide";
inline constexpr const char *AudioEnabledKey = "audioEnabled";
inline constexpr const char *AudioVolumeKey = "audioVolume";

inline constexpr const char *PopulationSizeKey = "populationSize";
inline constexpr const char *MaximumIterationTimeKey = "maximumIterationTime";
inline constexpr const char *EliteRatioKey = "eliteRatio";
inline constexpr const char *SimulationSpeedUpKey = "simulationSpeedUp";
inline constexpr const char *AngularSpeedKey = "angularSpeedDegrees";
inline constexpr const char *SeedOffsetKey = "seedOffset";
inline constexpr const char *AsyncTrainingKey = "asyncTraining";

} // namespace sailboat_simulation

#endif
