

#include "ComponentInspectors.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "imgui.h"
#include "Components/AnimationComponent.h"
#include "Components/AttributesComponent.h"
#include "Components/BoxColliderComponent.h"
#include "Components/HealthComponent.h"
#include "Components/KeyboardControlledComponent.h"
#include "Components/MovementTypeComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/SoftCollisionComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"

namespace
{
int GetSpriteSheetColumnCount(const TextureInfo& textureInfo)
{
    if (textureInfo.sprite.frameWidth <= 0)
    {
        return 1;
    }

    return std::max(1, static_cast<int>(textureInfo.pixelWidth) / textureInfo.sprite.frameWidth);
}

int GetSpriteSheetFrameCount(const TextureInfo& textureInfo)
{
    if (textureInfo.sprite.frameWidth <= 0 || textureInfo.sprite.frameHeight <= 0)
    {
        return 1;
    }

    const int columns = std::max(1, static_cast<int>(textureInfo.pixelWidth) / textureInfo.sprite.frameWidth);
    const int rows = std::max(1, static_cast<int>(textureInfo.pixelHeight) / textureInfo.sprite.frameHeight);
    return columns * rows;
}

void ApplySpriteSheetFrame(SpriteComponent& sprite, const TextureInfo& textureInfo, int frame)
{
    const int frameCount = GetSpriteSheetFrameCount(textureInfo);
    frame = std::clamp(frame, 0, frameCount - 1);

    const int columns = GetSpriteSheetColumnCount(textureInfo);
    const int frameWidth = std::max(1, textureInfo.sprite.frameWidth);
    const int frameHeight = std::max(1, textureInfo.sprite.frameHeight);

    sprite.srcRect.x = static_cast<float>((frame % columns) * frameWidth);
    sprite.srcRect.y = static_cast<float>((frame / columns) * frameHeight);
    sprite.srcRect.w = static_cast<float>(frameWidth);
    sprite.srcRect.h = static_cast<float>(frameHeight);
}

int GetCurrentSpriteSheetFrame(const SpriteComponent& sprite, const TextureInfo& textureInfo)
{
    const int frameWidth = std::max(1, textureInfo.sprite.frameWidth);
    const int frameHeight = std::max(1, textureInfo.sprite.frameHeight);
    const int columns = GetSpriteSheetColumnCount(textureInfo);

    const int col = std::max(0, static_cast<int>(sprite.srcRect.x) / frameWidth);
    const int row = std::max(0, static_cast<int>(sprite.srcRect.y) / frameHeight);
    return row * columns + col;
}

void ApplyTextureDefault(SpriteComponent& sprite, const TextureInfo& textureInfo)
{
    sprite.assetId = textureInfo.id;
    sprite.width = std::max(1, textureInfo.sprite.defaultDisplayWidth);
    sprite.height = std::max(1, textureInfo.sprite.defaultDisplayHeight);

    if (textureInfo.sprite.mode == TextureSpriteMode::SpriteSheet)
    {
        ApplySpriteSheetFrame(sprite, textureInfo, textureInfo.sprite.defaultFrame);
        return;
    }

    sprite.srcRect.x = 0.0f;
    sprite.srcRect.y = 0.0f;
    sprite.srcRect.w = textureInfo.pixelWidth;
    sprite.srcRect.h = textureInfo.pixelHeight;
}
}

void EditorInspector::DrawTransform(Entity selectedEntity)
{
    if (!selectedEntity.HasComponent<TransformComponent>())
    {
        return;
    }

    auto& transform = selectedEntity.GetComponent<TransformComponent>();

    ImGui::SeparatorText("Transform");

    float position[2] = {
        transform.position.x,
        transform.position.y
    };

    if (ImGui::DragFloat2("Position", position, 1.0f, 0.0f, 0.0f, "%.3f"))
    {
        transform.position.x = position[0];
        transform.position.y = position[1];
    }

    float scale[2] = {
        transform.scale.x,
        transform.scale.y
    };

    if (ImGui::DragFloat2("Scale", scale, 0.01f, 0.0f, 0.0f, "%.3f"))
    {
        transform.scale.x = scale[0];
        transform.scale.y = scale[1];
    }

    float rotation = static_cast<float>(transform.rotation);
    if (ImGui::DragFloat("Rotation", &rotation, 1.0f, 0.0f, 0.0f, "%.3f"))
    {
        transform.rotation = rotation;
    }
}

void EditorInspector::DrawRigidBody(Entity selectedEntity)
{
    if (!selectedEntity.HasComponent<RigidBodyComponent>())
    {
        return;
    }

    auto& rigidBody = selectedEntity.GetComponent<RigidBodyComponent>();

    float velocity[2] = {
        rigidBody.velocity.x,
        rigidBody.velocity.y
    };

    ImGui::SeparatorText("Rigid Body");
    if (ImGui::DragFloat2("Velocity", velocity, 1.0f, 0.0f, 0.0f, "%.4f"))
    {
        rigidBody.velocity.x = velocity[0];
        rigidBody.velocity.y = velocity[1];
    }
}

void EditorInspector::DrawSoftCollision(Entity selectedEntity)
{
    if (!selectedEntity.HasComponent<SoftCollisionComponent>())
    {
        return;
    }

    auto& softCollision = selectedEntity.GetComponent<SoftCollisionComponent>();

    float radius = softCollision.radius;
    float pushStrength = softCollision.pushStrength;
    bool immovable = softCollision.immovable;

    ImGui::SeparatorText("Soft Collision");
    if (ImGui::DragFloat("Radius", &radius, 1.0f, 0.0f, 256.0f, "%.2f"))
    {
        softCollision.radius = std::max(0.0f, radius);
    }

    if (ImGui::DragFloat("Push Strength", &pushStrength, 0.05f, 0.0f, 1.0f, "%.2f"))
    {
        softCollision.pushStrength = std::clamp(pushStrength, 0.0f, 1.0f);
    }

    if (ImGui::Checkbox("Immovable", &immovable))
    {
        softCollision.immovable = immovable;
    }
}

void EditorInspector::DrawKeyboardControl(Entity selectedEntity)
{
    if (!selectedEntity.HasComponent<KeyboardControlledComponent>())
    {
        return;
    }

    auto& keyboardControl = selectedEntity.GetComponent<KeyboardControlledComponent>();

    float speed = std::max({
        std::abs(keyboardControl.upVelocity.y),
        std::abs(keyboardControl.rightVelocity.x),
        std::abs(keyboardControl.downVelocity.y),
        std::abs(keyboardControl.leftVelocity.x)
    });

    float upVelocity[2] = {keyboardControl.upVelocity.x, keyboardControl.upVelocity.y};
    float rightVelocity[2] = {keyboardControl.rightVelocity.x, keyboardControl.rightVelocity.y};
    float downVelocity[2] = {keyboardControl.downVelocity.x, keyboardControl.downVelocity.y};
    float leftVelocity[2] = {keyboardControl.leftVelocity.x, keyboardControl.leftVelocity.y};

    ImGui::SeparatorText("Keyboard Control");

    if (ImGui::DragFloat("Move Speed", &speed, 1.0f, 0.0f, 0.0f, "%.2f"))
    {
        speed = std::max(0.0f, speed);
        keyboardControl.upVelocity = glm::vec2(0.0f, -speed);
        keyboardControl.rightVelocity = glm::vec2(speed, 0.0f);
        keyboardControl.downVelocity = glm::vec2(0.0f, speed);
        keyboardControl.leftVelocity = glm::vec2(-speed, 0.0f);
    }

    if (ImGui::TreeNode("Advanced Velocities"))
    {
        if (ImGui::DragFloat2("Up", upVelocity, 1.0f, 0.0f, 0.0f, "%.2f"))
        {
            keyboardControl.upVelocity.x = upVelocity[0];
            keyboardControl.upVelocity.y = upVelocity[1];
        }

        if (ImGui::DragFloat2("Right", rightVelocity, 1.0f, 0.0f, 0.0f, "%.2f"))
        {
            keyboardControl.rightVelocity.x = rightVelocity[0];
            keyboardControl.rightVelocity.y = rightVelocity[1];
        }

        if (ImGui::DragFloat2("Down", downVelocity, 1.0f, 0.0f, 0.0f, "%.2f"))
        {
            keyboardControl.downVelocity.x = downVelocity[0];
            keyboardControl.downVelocity.y = downVelocity[1];
        }

        if (ImGui::DragFloat2("Left", leftVelocity, 1.0f, 0.0f, 0.0f, "%.2f"))
        {
            keyboardControl.leftVelocity.x = leftVelocity[0];
            keyboardControl.leftVelocity.y = leftVelocity[1];
        }

        ImGui::TreePop();
    }
}

void EditorInspector::DrawSprite(Entity selectedEntity, AssetRegistry& assetRegistry)
{
    if (!selectedEntity.HasComponent<SpriteComponent>())
    {
        return;
    }

    auto& sprite = selectedEntity.GetComponent<SpriteComponent>();

    int width = sprite.width;
    int height = sprite.height;
    int zIndex = sprite.zIndex;
    bool isFixed = sprite.isFixed;
    SDL_FRect srcRect = sprite.srcRect;
    const char* flipModes[] = {"None", "Horizontal", "Vertical", "Horizontal + Vertical"};
    int currentFlipIndex = 0;

    switch (sprite.flip)
    {
    case SDL_FLIP_NONE:
        currentFlipIndex = 0;
        break;
    case SDL_FLIP_HORIZONTAL:
        currentFlipIndex = 1;
        break;
    case SDL_FLIP_VERTICAL:
        currentFlipIndex = 2;
        break;
    case SDL_FLIP_HORIZONTAL_AND_VERTICAL:
        currentFlipIndex = 3;
        break;
    }

    std::vector<std::string> textureIds = assetRegistry.GetTextureIds();

    ImGui::SeparatorText("Sprite");
    const char* texturePreview = sprite.assetId.empty() ? "<none>" : sprite.assetId.c_str();

    if (ImGui::BeginCombo("Texture", texturePreview))
    {
        for (const auto& textureId : textureIds)
        {
            bool isSelected = (sprite.assetId == textureId);

            if (ImGui::Selectable(textureId.c_str(), isSelected))
            {
                if (const TextureInfo* textureInfo = assetRegistry.GetTextureInfo(textureId))
                {
                    ApplyTextureDefault(sprite, *textureInfo);
                }
            }

            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    const TextureInfo* textureInfo = assetRegistry.GetTextureInfo(sprite.assetId);
    if (textureInfo)
    {
        ImGui::Text(
            "Asset Type: %s",
            textureInfo->sprite.mode == TextureSpriteMode::SpriteSheet ? "Sprite Sheet" : "Single Image"
        );

        if (ImGui::Button("Use Asset Default"))
        {
            ApplyTextureDefault(sprite, *textureInfo);
            width = sprite.width;
            height = sprite.height;
            srcRect = sprite.srcRect;
        }

        if (textureInfo->sprite.mode == TextureSpriteMode::SpriteSheet)
        {
            int frame = GetCurrentSpriteSheetFrame(sprite, *textureInfo);
            const int frameCount = GetSpriteSheetFrameCount(*textureInfo);

            if (ImGui::DragInt("Frame", &frame, 1.0f, 0, frameCount - 1))
            {
                frame = std::clamp(frame, 0, frameCount - 1);
                ApplySpriteSheetFrame(sprite, *textureInfo, frame);
                srcRect = sprite.srcRect;
            }

            ImGui::Text(
                "Frame Size: %d x %d, Frames: %d",
                textureInfo->sprite.frameWidth,
                textureInfo->sprite.frameHeight,
                frameCount
            );
        }
    }

    if (ImGui::DragInt("Width", &width, 1.0f, 1, 0))
    {
        width = std::max(1, width);
        sprite.width = width;

        if (sprite.srcRect.w <= 0.0f)
        {
            sprite.srcRect.w = static_cast<float>(width);
        }
    }
    if (ImGui::DragInt("Height", &height, 1.0f, 1, 0))
    {
        height = std::max(1, height);
        sprite.height = height;

        if (sprite.srcRect.h <= 0.0f)
        {
            sprite.srcRect.h = static_cast<float>(height);
        }
    }
    if (ImGui::DragInt("Z Index", &zIndex, 1.0f))
    {
        sprite.zIndex = zIndex;
    }
    if (ImGui::Checkbox("Fixed", &isFixed))
    {
        sprite.isFixed = isFixed;
    }
    if (ImGui::Combo("Flip", &currentFlipIndex, flipModes, IM_ARRAYSIZE(flipModes)))
    {
        switch (currentFlipIndex)
        {
        case 0:
            sprite.flip = SDL_FLIP_NONE;
            break;
        case 1:
            sprite.flip = SDL_FLIP_HORIZONTAL;
            break;
        case 2:
            sprite.flip = SDL_FLIP_VERTICAL;
            break;
        case 3:
            sprite.flip = SDL_FLIP_HORIZONTAL_AND_VERTICAL;
            break;
        }
    }
    if (ImGui::DragFloat2("Src Rect Pos", &srcRect.x, 1.0f, 0.0f, 0.0f, "%.3f"))
    {
        sprite.srcRect.x = srcRect.x;
        sprite.srcRect.y = srcRect.y;
    }
    if (ImGui::DragFloat2("Src Rect Size", &srcRect.w, 1.0f, 0.0f, 0.0f, "%.3f"))
    {
        sprite.srcRect.w = srcRect.w;
        sprite.srcRect.h = srcRect.h;
    }
}

void EditorInspector::DrawHealth(Entity selectedEntity)
{
    if (!selectedEntity.HasComponent<HealthComponent>())
    {
        return;
    }

    auto& health = selectedEntity.GetComponent<HealthComponent>();

    int healthPercentage = health.healthPercentage;

    ImGui::SeparatorText("Health");
    if (ImGui::DragInt("Health", &healthPercentage, 1.0f, 0, 100))
    {
        health.healthPercentage = healthPercentage;
    }
}

void EditorInspector::DrawAttributes(Entity selectedEntity)
{
    if (!selectedEntity.HasComponent<AttributesComponent>())
    {
        return;
    }

    auto& attributes = selectedEntity.GetComponent<AttributesComponent>();
    if (!attributes.values.is_object())
    {
        attributes.values = nlohmann::json::object();
    }

    static char newAttributeName[64] = {};
    static int newAttributeType = 0;
    static double newNumberValue = 0.0;
    static bool newBoolValue = false;
    static char newStringValue[128] = {};

    ImGui::SeparatorText("Attributes");

    std::string keyToRemove;

    for (auto it = attributes.values.begin(); it != attributes.values.end(); ++it)
    {
        const std::string key = it.key();
        nlohmann::json& value = it.value();

        ImGui::PushID(key.c_str());
        ImGui::TextUnformatted(key.c_str());
        ImGui::SameLine(140.0f);

        if (value.is_boolean())
        {
            bool boolValue = value.get<bool>();
            if (ImGui::Checkbox("##value", &boolValue))
            {
                value = boolValue;
            }
        }
        else if (value.is_number_integer())
        {
            int intValue = value.get<int>();
            if (ImGui::DragInt("##value", &intValue, 1.0f))
            {
                value = intValue;
            }
        }
        else if (value.is_number())
        {
            float floatValue = value.get<float>();
            if (ImGui::DragFloat("##value", &floatValue, 0.1f, 0.0f, 0.0f, "%.3f"))
            {
                value = floatValue;
            }
        }
        else if (value.is_string())
        {
            char stringValue[128] = {};
            std::snprintf(stringValue, sizeof(stringValue), "%s", value.get<std::string>().c_str());

            if (ImGui::InputText("##value", stringValue, sizeof(stringValue)))
            {
                value = stringValue;
            }
        }
        else
        {
            ImGui::TextDisabled("%s", value.dump().c_str());
        }

        ImGui::SameLine();
        if (ImGui::SmallButton("Remove"))
        {
            keyToRemove = key;
        }

        ImGui::PopID();
    }

    if (!keyToRemove.empty())
    {
        attributes.values.erase(keyToRemove);
    }

    if (ImGui::TreeNode("Add Attribute"))
    {
        ImGui::InputText("Name", newAttributeName, sizeof(newAttributeName));

        const char* attributeTypes[] = {"Number", "Integer", "Boolean", "String"};
        ImGui::Combo("Type", &newAttributeType, attributeTypes, IM_ARRAYSIZE(attributeTypes));

        if (newAttributeType == 0)
        {
            float value = static_cast<float>(newNumberValue);
            if (ImGui::DragFloat("Default Value", &value, 0.1f, 0.0f, 0.0f, "%.3f"))
            {
                newNumberValue = value;
            }
        }
        else if (newAttributeType == 1)
        {
            int value = static_cast<int>(newNumberValue);
            if (ImGui::DragInt("Default Value", &value, 1.0f))
            {
                newNumberValue = value;
            }
        }
        else if (newAttributeType == 2)
        {
            ImGui::Checkbox("Default Value", &newBoolValue);
        }
        else
        {
            ImGui::InputText("Default Value", newStringValue, sizeof(newStringValue));
        }

        if (ImGui::Button("Add"))
        {
            const std::string name = newAttributeName;

            if (!name.empty() && !attributes.values.contains(name))
            {
                if (newAttributeType == 0)
                {
                    attributes.values[name] = newNumberValue;
                }
                else if (newAttributeType == 1)
                {
                    attributes.values[name] = static_cast<int>(newNumberValue);
                }
                else if (newAttributeType == 2)
                {
                    attributes.values[name] = newBoolValue;
                }
                else
                {
                    attributes.values[name] = newStringValue;
                }

                newAttributeName[0] = '\0';
                newStringValue[0] = '\0';
                newNumberValue = 0.0;
                newBoolValue = false;
            }
        }

        ImGui::TreePop();
    }
}

void EditorInspector::DrawBoxCollider(Entity selectedEntity)
{
    if (!selectedEntity.HasComponent<BoxColliderComponent>())
    {
        return;
    }

    auto& boxCollider = selectedEntity.GetComponent<BoxColliderComponent>();

    int size[2] = {boxCollider.width, boxCollider.height};
    glm::vec2 offset = boxCollider.offset;
    bool matchSpriteSize = boxCollider.matchSpriteSize;
    bool rotateWithTransform = boxCollider.rotateWithTransform;

    ImGui::SeparatorText("BoxCollider");

    if (ImGui::Checkbox("Match Sprite Size", &matchSpriteSize))
    {
        boxCollider.matchSpriteSize = matchSpriteSize;
    }

    if (boxCollider.matchSpriteSize && selectedEntity.HasComponent<SpriteComponent>())
    {
        const auto& sprite = selectedEntity.GetComponent<SpriteComponent>();
        boxCollider.width = std::max(1, sprite.width);
        boxCollider.height = std::max(1, sprite.height);
        size[0] = boxCollider.width;
        size[1] = boxCollider.height;
    }

    if (boxCollider.matchSpriteSize)
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::DragInt2("Size", size, 1.0f, 1, 4096))
    {
        size[0] = std::max(1, size[0]);
        size[1] = std::max(1, size[1]);
        boxCollider.width = size[0];
        boxCollider.height = size[1];
    }

    if (boxCollider.matchSpriteSize)
    {
        ImGui::EndDisabled();
    }

    if (ImGui::Checkbox("Rotate With Transform", &rotateWithTransform))
    {
        boxCollider.rotateWithTransform = rotateWithTransform;
    }

    if (ImGui::DragFloat2("Offset", &offset.x, 1.0f, 0.0f, 0.0f, "%.3f"))
    {
        boxCollider.offset = offset;
    }
}

void EditorInspector::DrawMovementType(Entity selectedEntity)
{
    if (!selectedEntity.HasComponent<MovementTypeComponent>())
    {
        return;
    }

    auto& movement = selectedEntity.GetComponent<MovementTypeComponent>();

    const char* movementTypes[] = {"Land", "Water", "Air"};
    int currentIndex = 0;

    switch (movement.type)
    {
    case MovementType::Land:
        currentIndex = 0;
        break;
    case MovementType::Water:
        currentIndex = 1;
        break;
    case MovementType::Air:
        currentIndex = 2;
        break;
    }

    ImGui::SeparatorText("MovementType");
    if (ImGui::Combo("Type", &currentIndex, movementTypes, IM_ARRAYSIZE(movementTypes)))
    {
        switch (currentIndex)
        {
        case 0:
            movement.type = MovementType::Land;
            break;
        case 1:
            movement.type = MovementType::Water;
            break;
        case 2:
            movement.type = MovementType::Air;
            break;
        }
    }
}

void EditorInspector::DrawAnimation(Entity selectedEntity)
{
    if (!selectedEntity.HasComponent<AnimationComponent>())
    {
        return;
    }

    auto& animation = selectedEntity.GetComponent<AnimationComponent>();

    int numFrames = animation.numFrames;
    int currentFrame = animation.currentFrame;
    int frameSpeedRate = animation.frameSpeedRate;
    bool isLoop = animation.isLoop;
    int startTime = animation.startTime;

    ImGui::SeparatorText("Animation");

    if (ImGui::DragInt("Num Frames", &numFrames, 1.0f, 1, 0))
    {
        animation.numFrames = numFrames;
    }
    if (ImGui::DragInt("Current Frame", &currentFrame, 1.0f, 0, 0))
    {
        animation.currentFrame = currentFrame;
    }
    if (ImGui::DragInt("Frame Speed Rate", &frameSpeedRate, 1.0f, 0, 0))
    {
        animation.frameSpeedRate = frameSpeedRate;
    }
    if (ImGui::Checkbox("Loop", &isLoop))
    {
        animation.isLoop = isLoop;
    }
    if (ImGui::DragInt("Start Time", &startTime, 1.0f, 0, 0))
    {
        animation.startTime = startTime;
    }
}
