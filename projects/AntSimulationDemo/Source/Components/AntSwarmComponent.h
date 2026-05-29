#ifndef PIPEFRAME_ANT_SWARM_COMPONENT_H
#define PIPEFRAME_ANT_SWARM_COMPONENT_H

#include "Reflection/ComponentAnnotations.h"

PF_COMPONENT()
struct AntSwarmComponent
{
    PF_PROPERTY(PF::Edit, PF::Save, 0, 10000, 1)
    int maxAgents = 100;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 500.0f, 1.0f)
    float speed = 52.0f;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 2000.0f, 1.0f)
    float maxEnergy = 600.0f;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 1.0f, 0.01f)
    float refillEnergyRatio = 0.5f;

    PF_PROPERTY(PF::Edit, PF::Save, 1.0f, 128.0f, 1.0f)
    float pickupRadius = 18.0f;

    PF_PROPERTY(PF::Edit, PF::Save, 1.0f, 128.0f, 1.0f)
    float dropoffRadius = 34.0f;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 256.0f, 1.0f, PF::DisplayName("Sample Distance Min"))
    float sensorDistanceMin = 12.0f;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 512.0f, 1.0f, PF::DisplayName("Sample Distance Max"))
    float sensorDistanceMax = 64.0f;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 6.28f, 0.01f, PF::DisplayName("Follower FOV"))
    float followerFovRadians = 2.35619f;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 6.28f, 0.01f, PF::DisplayName("Explorer FOV"))
    float explorerFovRadians = 1.5708f;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 1.0f, 0.01f)
    float explorerRatio = 0.1f;

    PF_PROPERTY(PF::Edit, PF::Save, 1, 256, 1, PF::DisplayName("Follower Samples"))
    int followerSampleCount = 64;

    PF_PROPERTY(PF::Edit, PF::Save, 1, 128, 1, PF::DisplayName("Explorer Samples"))
    int explorerSampleCount = 8;

    PF_PROPERTY(PF::Edit, PF::Save, 1.0f, 256.0f, 1.0f)
    float markerDistance = 24.0f;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 5000.0f, 0.05f, PF::DisplayName("Trail Deposit"))
    float trailDepositAmount = 1000.0f;

    PF_PROPERTY(PF::Edit, PF::Save, 1.0f, 96.0f, 1.0f)
    float trailRadius = 6.0f;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 5000.0f, 1.0f, PF::DisplayName("Blocked Degrade"))
    float blockedDegradeAmount = 1000.0f;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 2.0f, 0.01f)
    float foodTrailDecayPerSecond = 0.035f;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 2.0f, 0.01f)
    float homeTrailDecayPerSecond = 0.035f;

    PF_PROPERTY(PF::Edit, PF::Save, 0.1f, 20.0f, 0.1f, PF::DisplayName("Max Turn Speed"))
    float maxTurnRadiansPerSecond = 5.5f;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 48.0f, 1.0f)
    float agentRadius = 7.0f;

    PF_PROPERTY(PF::Edit, PF::Save, 0.0f, 1.0f, 0.05f)
    float collisionPushStrength = 0.12f;

    PF_PROPERTY(PF::Edit, PF::Save, 0, 8, 1)
    int collisionIterations = 3;
};

#endif // PIPEFRAME_ANT_SWARM_COMPONENT_H
