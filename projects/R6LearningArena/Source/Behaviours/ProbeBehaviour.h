#pragma once
#include <PipeFrame/ECS/Scene.h>
#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/Components/KinematicBody2DComponent.h>
#include <PipeFrame/Environment/EnvironmentQueries.h>
#include "Components/ProbeSettings.h"
#include <cmath>
class ProbeBehaviour final : public pipeframe::Behaviour {
    void FixedUpdate(float) override {
        auto *settings=GetComponent<ProbeSettings>();
        auto *pose=GetComponent<pipeframe::Transform2DComponent>();
        auto *body=GetComponent<pipeframe::KinematicBody2DComponent>();
        auto *queries=GetService<pipeframe::EnvironmentQueries>();
        if(!body)return;
        if(!settings||!pose||!queries){body->velocity={};return;}
        const auto length=std::hypot(settings->direction.x,settings->direction.y);
        if(!std::isfinite(length)||length<=0){body->velocity={};return;}
        const auto direction=settings->direction/length;
        const auto hit=queries->RaycastEnvironment(pose->position,direction,settings->range,
                                                   std::uint32_t(body->layerMask));
        settings->nearest=hit?hit->distance:-1;
        settings->blocked=hit && hit->distance<=settings->stopDistance+body->radius;
        body->velocity=settings->blocked?pipeframe::Vector2f{}:direction*settings->speed;
    }
};
