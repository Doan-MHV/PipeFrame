# PipeFrame learning and experiment runtime

Milestone 17F moves generic learning and experiment infrastructure out of
SailBoat and into the backend-neutral `PipeFrame::Learning` module. The module
does not include SFML, Torch, CUDA, sailing types, or Workbench UI types.

## Public layers

| Layer | Public types | Responsibility |
|---|---|---|
| Network model | `Genome`, `DirectedAcyclicGraph`, `Network`, `NetworkGenerator` | Versioned topology, activation, compilation, execution, and persistence |
| Evolution | `MutationSettings`, `GenomeMutator`, `GenomeSelector`, `EvolutionExperiment` | Deterministic mutation, weighted selection, elites, generations, best model, and a reusable non-domain trainer |
| Experiment lifecycle | `ExperimentClock`, `ExperimentHistory`, `CheckpointMetadata`, `BackgroundEvaluation` | Time/progress, named metrics, run metadata, deterministic checkpoints, and asynchronous work |
| Integration contracts | `Policy`, `NetworkPolicy`, `Environment`, `Trainer` | Domain-neutral observations, actions, scoring, control, and checkpoint access |
| Inspection | `InferenceSnapshot`, `InferenceNode`, `InferenceEdge` | Live sums, activated values, biases, roles, signed weights, propagated edge values, enabled state, and topology |

`DeterministicRandom` supplies stable seeded streams. Evolution derives mutation
and selection streams from the root seed, run, and generation, so a checkpoint
can reproduce the next population without serializing a platform-specific random
engine object.

## SailBoat integration

`SailBoatAgent` owns a PipeFrame genome and compiled network. The SailBoat
trainer retains only race evaluation and boat reset logic; it uses PipeFrame for
mutation, selection, the generation clock, history, checkpoint metadata, and
background evaluation. Checkpoints record the population, best genome and score,
run, generation, seed, evolution settings, evaluation timing, and history. The
continuation regression deliberately initializes the destination with different
evolution settings, loads a checkpoint, runs both source and resumed trainers,
and compares every resulting genome.

The dashboard reads `Network::GetInferenceSnapshot()` after live inference. It
does not reconstruct runtime activation state from a SailBoat-specific genome.
Each edge also exposes its current propagated value (`source activation *
weight`), matching the value used by Pezzza's animated network renderer. Final
panel placement, labels, and Pezzza styling remain visual work in 17I.

## Pezzza fidelity audit

The 17F implementation was checked against `SailBoatPezza/src/neat`,
`SailBoatPezza/src/training`, and the complete transcript plus sampled frames of
“Using Evolution and Neural Networks to optimize Sailboat Races.” The resulting
contracts preserve these reference behaviors:

- Four local-frame inputs describe the target and wind; one tanh output controls
  angular velocity.
- Evaluation rewards early checkpoint progress and adds the finish bonus scaled
  by race time.
- Evolution ranks the population, retains the configured elite fraction, uses
  score-weighted parent selection, mutates duplicates, and adds hidden nodes by
  splitting connections.
- Connection mutation samples its source only from inputs and hidden nodes and
  its target only from hidden and output nodes, using the same single-attempt
  behavior as Pezzza's mutator.
- Runtime inspection reports node activations and the live signed signal on each
  connection, which is what Pezzza uses to animate network nodes and edge widths.
- Long-running training can switch to background evaluation and can be resumed
  without losing the best score or changing the saved evolution configuration.

The Ant Simulator 2 video was also reviewed because it defines behavioral parity
for the next milestone. Its life-cycle, colony, marker-ownership, and deterministic
recording rules are recorded with timestamps in the Ant parity matrix. They are
17G simulation requirements, not learning-runtime behavior.

## Area and optional PPO

Area can implement `Environment` with its observation vector, action handling,
terminal condition, and reward/score. A PPO policy adapter implements `Policy`
and exposes whatever inspection data it supports. A PPO trainer implements
`Trainer` and uses the shared clock, statistics, history, checkpoint metadata,
and background coordination.

Torch and CUDA stay in that optional Area/PPO adapter. They are not dependencies
of `PipeFrame::Learning`. This boundary allows NEAT, PPO, scripted policies, and
future trainers to share editor and experiment workflows without sharing their
algorithm libraries.

## Acceptance evidence

`LearningExperimentTests` is the non-boat consumer. It evolves a scalar policy,
checks deterministic populations, saves and resumes a checkpoint, continues the
same generation on both instances, exercises the environment/policy contracts,
and inspects live topology including active propagated values and a disabled
negative edge. SailBoat's NEAT,
population, asynchronous, renderer, dashboard, and performance regressions guard
the migrated behavior. Dependency lint rejects backend types in all public
learning headers and rejects a returning project-local SailBoat NEAT/history
implementation.
