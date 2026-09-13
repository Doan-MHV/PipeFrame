#include "SceneDocument.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_set>
#include <utility>

namespace pipeframe::editor {

const std::vector<SceneObjectData> &
SceneDocument::GetObjects() const {
    return objects;
}

const SceneSettings &SceneDocument::GetSettings() const { return settings; }

bool SceneDocument::SetSettings(SceneSettings value) {
    if (value.lengthUnit.empty() || value.angleUnit.empty() || value.coordinateSystem.empty() || value == settings)
        return false;
    settings = std::move(value); dirty = true; return true;
}

const std::vector<SceneConnectionData> &SceneDocument::GetConnections() const { return connections; }

bool SceneDocument::AddConnection(SceneConnectionData connection) {
    if (connection.id == 0 || connection.from.objectId == 0 || connection.to.objectId == 0 ||
        connection.from.attachmentId.empty() || connection.to.attachmentId.empty() ||
        connection.from == connection.to || !FindObject(connection.from.objectId) || !FindObject(connection.to.objectId) ||
        std::ranges::any_of(connections, [&](const auto &item) {
            return item.id == connection.id || (item.kind == connection.kind &&
                ((item.from == connection.from && item.to == connection.to) ||
                 (item.from == connection.to && item.to == connection.from)));
        })) return false;
    connections.push_back(std::move(connection)); dirty = true; return true;
}

bool SceneDocument::RemoveConnection(const std::uint64_t connectionId) {
    const auto count = std::erase_if(connections, [=](const auto &item) { return item.id == connectionId; });
    dirty |= count > 0; return count > 0;
}

const SceneObjectData *SceneDocument::FindObject(
    const SceneObjectId objectId) const {

    const auto iterator =
        std::find_if(
            objects.begin(),
            objects.end(),
            [objectId](const SceneObjectData &object) {
                return object.id == objectId;
            });

    return iterator != objects.end()
               ? &*iterator
               : nullptr;
}

std::vector<const SceneObjectData *> SceneDocument::GetChildren(const SceneObjectId parentId) const {
    std::vector<const SceneObjectData *> result;
    for (const auto &object : objects) if (object.parentId == parentId) result.push_back(&object);
    std::ranges::sort(result, {}, [](const SceneObjectData *object) { return object->siblingOrder; });
    return result;
}

SceneTransform SceneDocument::GetWorldTransform(const SceneObjectId objectId) const {
    SceneTransform result{};
    result.scale = {1.0f, 1.0f};
    std::vector<const SceneObjectData *> chain;
    std::unordered_set<SceneObjectId> visited;
    for (const SceneObjectData *object = FindObject(objectId); object && visited.insert(object->id).second;
         object = object->parentId == 0 ? nullptr : FindObject(object->parentId)) chain.push_back(object);
    for (auto iterator = chain.rbegin(); iterator != chain.rend(); ++iterator) {
        const auto &local = (*iterator)->transform;
        const float radians = result.rotation * 3.14159265f / 180.0f;
        const Vector2f scaled{local.position.x * result.scale.x, local.position.y * result.scale.y};
        const Vector2f rotated{scaled.x * std::cos(radians) - scaled.y * std::sin(radians),
                               scaled.x * std::sin(radians) + scaled.y * std::cos(radians)};
        result.position += rotated;
        result.rotation += local.rotation;
        result.scale.x *= local.scale.x; result.scale.y *= local.scale.y;
    }
    return result;
}

SceneObjectData *SceneDocument::FindMutableObject(
    const SceneObjectId objectId) {

    const auto iterator =
        std::find_if(
            objects.begin(),
            objects.end(),
            [objectId](const SceneObjectData &object) {
                return object.id == objectId;
            });

    return iterator != objects.end()
               ? &*iterator
               : nullptr;
}

SceneObjectId SceneDocument::CreateObject(
    std::string name,
    SceneObjectTypeId typeId,
    const SceneTransform transform,
    PropertyMap properties) {

    if (typeId.empty()) {
        return 0;
    }

    const SceneObjectId objectId =
        nextObjectId++;

    SceneObjectData object{
        objectId,
        std::move(name),
        std::move(typeId),
        transform,
        std::move(properties),
    };
    SynchronizeTransformComponent(object);
    objects.push_back(std::move(object));

    dirty = true;

    return objectId;
}

SceneObjectId SceneDocument::CreateObject(SceneObjectData object) {
    if (object.typeId.empty()) return 0;
    object.id = nextObjectId++;
    SynchronizeTransformComponent(object);
    objects.push_back(std::move(object));
    dirty = true;
    return objects.back().id;
}

bool SceneDocument::RestoreObject(
    SceneObjectData object) {

    if (object.id == 0 ||
        object.typeId.empty() ||
        FindObject(object.id) != nullptr) {

        return false;
    }

    nextObjectId =
        std::max(
            nextObjectId,
            object.id + 1);

    SynchronizeTransformComponent(object);
    objects.push_back(std::move(object));

    dirty = true;

    return true;
}

bool SceneDocument::RemoveObject(
    const SceneObjectId objectId) {

    std::unordered_set<SceneObjectId> removals{objectId};
    bool expanded=true;
    while(expanded){ expanded=false; for(const auto &object:objects) if(removals.contains(object.parentId)&&removals.insert(object.id).second) expanded=true; }
    const std::size_t removedCount = std::erase_if(objects,[&](const SceneObjectData &object){return removals.contains(object.id);});
    std::erase_if(connections, [&](const SceneConnectionData &connection) {
        return removals.contains(connection.from.objectId) || removals.contains(connection.to.objectId);
    });

    if (removedCount == 0) {
        return false;
    }

    dirty = true;

    return true;
}

bool SceneDocument::RenameObject(const SceneObjectId objectId, std::string name) {
    auto *object=FindMutableObject(objectId); if(!object||name.empty()||object->name==name)return false;
    object->name=std::move(name); dirty=true; return true;
}

bool SceneDocument::IsDescendant(SceneObjectId candidate, const SceneObjectId ancestor) const {
    std::unordered_set<SceneObjectId> visited;
    while(candidate!=0&&visited.insert(candidate).second){ if(candidate==ancestor)return true; const auto *object=FindObject(candidate); candidate=object?object->parentId:0; }
    return false;
}

bool SceneDocument::SetParent(const SceneObjectId objectId, const SceneObjectId parentId, std::int32_t siblingOrder) {
    auto *object=FindMutableObject(objectId);
    if(!object||objectId==parentId||(parentId!=0&&!FindObject(parentId))||IsDescendant(parentId,objectId))return false;
    if(object->parentId==parentId&&siblingOrder<0)return false;
    object->parentId=parentId;
    if(siblingOrder<0) siblingOrder=static_cast<std::int32_t>(GetChildren(parentId).size());
    object->siblingOrder=siblingOrder; dirty=true; return true;
}

bool SceneDocument::SetLayer(SceneObjectId id,std::string layer){auto *o=FindMutableObject(id);if(!o||layer.empty()||o->layer==layer)return false;o->layer=std::move(layer);dirty=true;return true;}
bool SceneDocument::SetTags(SceneObjectId id,std::vector<std::string> tags){auto *o=FindMutableObject(id);if(!o)return false;std::erase_if(tags,[](const auto &v){return v.empty();});std::ranges::sort(tags);tags.erase(std::unique(tags.begin(),tags.end()),tags.end());if(o->tags==tags)return false;o->tags=std::move(tags);dirty=true;return true;}
bool SceneDocument::SetVisible(SceneObjectId id,bool value){auto *o=FindMutableObject(id);if(!o||o->visible==value)return false;o->visible=value;dirty=true;return true;}
bool SceneDocument::SetLocked(SceneObjectId id,bool value){auto *o=FindMutableObject(id);if(!o||o->locked==value)return false;o->locked=value;dirty=true;return true;}
bool SceneDocument::AddComponent(SceneObjectId id,SceneComponentData component){auto *o=FindMutableObject(id);if(!o||component.typeId.empty()||std::ranges::any_of(o->components,[&](const auto &c){return c.typeId==component.typeId;}))return false;o->components.push_back(std::move(component));dirty=true;return true;}
bool SceneDocument::RemoveComponent(SceneObjectId id,const std::string &type){auto *o=FindMutableObject(id);if(!o||type==Transform2DComponentTypeId)return false;const auto count=std::erase_if(o->components,[&](const auto &c){return c.typeId==type;});dirty|=count>0;return count>0;}
bool SceneDocument::SetComponentProperty(SceneObjectId id,const std::string &type,std::string key,PropertyValue value){
    auto *o=FindMutableObject(id);
    if(!o||key.empty())return false;
    auto c=std::ranges::find_if(o->components,[&](const auto &item){return item.typeId==type;});
    if(c==o->components.end())return false;
    auto old=c->properties.find(key);
    if(old!=c->properties.end()&&old->second==value)return false;
    if(type==Transform2DComponentTypeId){
        if(key=="position"&&std::holds_alternative<Vector2f>(value))o->transform.position=std::get<Vector2f>(value);
        else if(key=="rotation"&&std::holds_alternative<double>(value))o->transform.rotation=static_cast<float>(std::get<double>(value));
        else if(key=="scale"&&std::holds_alternative<Vector2f>(value))o->transform.scale=std::get<Vector2f>(value);
        else return false;
    }
    c->properties.insert_or_assign(std::move(key),std::move(value));
    dirty=true;
    return true;
}

bool SceneDocument::ReplaceObjectData(const SceneObjectId objectId, SceneObjectData replacement) {
    auto *object = FindMutableObject(objectId);
    if (!object || replacement.typeId.empty()) return false;
    replacement.id = objectId;
    if (replacement.parentId == objectId ||
        (replacement.parentId != 0 && !FindObject(replacement.parentId)) ||
        IsDescendant(replacement.parentId, objectId)) return false;
    SynchronizeTransformComponent(replacement);
    *object = std::move(replacement);
    dirty = true;
    return true;
}

bool SceneDocument::SetPrefabLinks(const SceneObjectId objectId,
                                   std::vector<PrefabInstanceLink> links) {
    auto *object = FindMutableObject(objectId);
    if (!object || object->prefabLinks == links) return false;
    object->prefabLinks = std::move(links);
    dirty = true;
    return true;
}

SceneObjectId SceneDocument::DuplicateRecursive(const SceneObjectId id,const SceneObjectId parent){
    const auto *source=FindObject(id);
    if(!source)return 0;
    SceneObjectData copy=*source;
    std::vector<SceneObjectId> childIds;
    for(const auto *child:GetChildren(id))childIds.push_back(child->id);
    copy.id=nextObjectId++;
    copy.name+=" Copy";
    copy.parentId=parent;
    objects.push_back(std::move(copy));
    const auto newId=objects.back().id;
    for(const auto childId:childIds)DuplicateRecursive(childId,newId);
    return newId;
}
SceneObjectId SceneDocument::DuplicateObject(const SceneObjectId id,const bool children){const auto *source=FindObject(id);if(!source)return 0;SceneObjectId result=0;if(children)result=DuplicateRecursive(id,source->parentId);else{SceneObjectData copy=*source;copy.id=nextObjectId++;copy.name+=" Copy";objects.push_back(std::move(copy));result=objects.back().id;}dirty|=result!=0;return result;}

std::vector<SceneObjectId> SceneDocument::FindObjects(const std::string &query,
                                                       const std::string &layer,
                                                       const std::string &tag,
                                                       const bool includeHidden) const {
    std::vector<SceneObjectId> result;
    std::string normalizedQuery = query;
    std::ranges::transform(normalizedQuery, normalizedQuery.begin(),
                           [](const unsigned char value) { return static_cast<char>(std::tolower(value)); });
    for (const auto &object : objects) {
        if ((!includeHidden && !object.visible) || (!layer.empty() && object.layer != layer) ||
            (!tag.empty() && std::ranges::find(object.tags, tag) == object.tags.end())) continue;
        std::string searchable = object.name + " " + object.typeId;
        std::ranges::transform(searchable, searchable.begin(),
                               [](const unsigned char value) { return static_cast<char>(std::tolower(value)); });
        if (!normalizedQuery.empty() && searchable.find(normalizedQuery) == std::string::npos) continue;
        result.push_back(object.id);
    }
    return result;
}

std::vector<std::string> SceneDocument::Validate() const {std::vector<std::string> errors;std::unordered_set<SceneObjectId> ids;for(const auto &o:objects){if(o.id==0||!ids.insert(o.id).second)errors.push_back("Duplicate or zero object ID.");if(o.parentId!=0&&!FindObject(o.parentId))errors.push_back("Object "+std::to_string(o.id)+" has a missing parent.");if(IsDescendant(o.parentId,o.id))errors.push_back("Object "+std::to_string(o.id)+" has a hierarchy cycle.");std::unordered_set<std::string> components;for(const auto &c:o.components)if(c.typeId.empty()||!components.insert(c.typeId).second)errors.push_back("Object "+std::to_string(o.id)+" has an invalid component.");}std::unordered_set<std::uint64_t> connectionIds;for(const auto &c:connections){if(c.id==0||!connectionIds.insert(c.id).second||!FindObject(c.from.objectId)||!FindObject(c.to.objectId)||c.from.attachmentId.empty()||c.to.attachmentId.empty())errors.push_back("Scene has an invalid connection.");}return errors;}

bool SceneDocument::SetTransform(
    const SceneObjectId objectId,
    const SceneTransform transform) {

    SceneObjectData *object =
        FindMutableObject(objectId);

    if (object == nullptr ||
        object->transform == transform) {

        return false;
    }

    object->transform = transform;
    SynchronizeTransformComponent(*object);
    dirty = true;

    return true;
}

bool SceneDocument::SetProperty(
    const SceneObjectId objectId,
    std::string key,
    PropertyValue value) {

    if (key.empty()) {
        return false;
    }

    SceneObjectData *object =
        FindMutableObject(objectId);

    if (object == nullptr) {
        return false;
    }

    const auto existing =
        object->properties.find(key);

    if (existing != object->properties.end() &&
        existing->second == value) {

        return false;
    }

    object->properties.insert_or_assign(
        std::move(key),
        std::move(value));

    dirty = true;

    return true;
}

bool SceneDocument::RemoveProperty(
    const SceneObjectId objectId,
    const std::string &key) {

    SceneObjectData *object =
        FindMutableObject(objectId);

    if (object == nullptr) {
        return false;
    }

    if (object->properties.erase(key) == 0) {
        return false;
    }

    dirty = true;

    return true;
}

const PropertyValue *SceneDocument::FindProperty(
    const SceneObjectId objectId,
    const std::string &key) const {

    const SceneObjectData *object =
        FindObject(objectId);

    if (object == nullptr) {
        return nullptr;
    }

    const auto property =
        object->properties.find(key);

    return property != object->properties.end()
               ? &property->second
               : nullptr;
}

void SceneDocument::Clear() {
    if (objects.empty()) {
        return;
    }

    objects.clear();
    connections.clear();
    settings = {};
    nextObjectId = 1;
    dirty = true;
}

bool SceneDocument::IsDirty() const {
    return dirty;
}

void SceneDocument::MarkClean() {
    dirty = false;
}

} // namespace pipeframe::editor
