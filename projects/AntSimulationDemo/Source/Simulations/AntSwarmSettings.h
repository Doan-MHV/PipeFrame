#ifndef PIPEFRAME_ANT_SWARM_SETTINGS_H
#define PIPEFRAME_ANT_SWARM_SETTINGS_H

struct AntSwarmSettings
{
    float speed = 52.0f;
    float maxEnergy = 600.0f;
    float refillEnergyRatio = 0.5f;
    float pickupRadius = 18.0f;
    float dropoffRadius = 34.0f;
    float sensorDistanceMin = 12.0f;
    float sensorDistanceMax = 64.0f;
    float followerFovRadians = 2.35619f;
    float explorerFovRadians = 1.5708f;
    float explorerRatio = 0.1f;
    int followerSampleCount = 64;
    int explorerSampleCount = 8;
    float markerDistance = 24.0f;
    float markerMaxIntensity = 1000.0f;
    float markerDecayPower = 4.0f;
    float trailDepositAmount = 1000.0f;
    float trailRadius = 6.0f;
    float blockedDegradeAmount = 1000.0f;
    float spawnRadius = 56.0f;
    float momentumWeight = 0.1f;
    float maxTurnRadiansPerSecond = 5.5f;
    float agentRadius = 7.0f;
    float collisionPushStrength = 0.12f;
    int collisionIterations = 3;
    double foodFieldDecayPerSecond = 0.035;
    double homeFieldDecayPerSecond = 0.035;
    double stuckTimeout = 7.5;
};

#endif // PIPEFRAME_ANT_SWARM_SETTINGS_H
