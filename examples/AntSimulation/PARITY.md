# Reference boundary

The rework preserves the tested PipeFrame Ant baseline, including deterministic replay,
colony/food/marker/encounter behavior, physics and procedural geometry. It does not establish
pixel-perfect or complete source equivalence to every Pezzza video/version.

The R3 source review recorded two existing differences: Pezzza's supplied simulation runs
physics before ant updates and colonies last, while PipeFrame begins with environment and
colony lifecycle before movement/behaviour. Pezzza measures travel using velocity×dt;
PipeFrame uses solved displacement and clamps remaining target distance. These are known
baseline policies, not evidence that the sources are identical. Changing them requires a
separate reference comparison and acceptance decision; R7 cleanup does not change them.

[Recorded source review and measurements](../../docs/rework/R3_RUNTIME_AUDIT.md) and
[reference material](../../docs/parity/milestone17/README.md) retain the provenance.
Soldier role/type creation verifies extension mechanics, not an implemented combat model.
The native editor uses PipeFrame controls; Unity/Flutter inspiration is an architectural
approach, not a claim to reproduce those products or Pezzza's UI exactly.
