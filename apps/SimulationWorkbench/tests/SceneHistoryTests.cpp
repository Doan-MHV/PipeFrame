#include "../Editor/SceneHistory.h"

#include <cstdlib>
#include <iostream>

namespace {

void Require(
    const bool condition,
    const char *message) {

    if (!condition) {
        std::cerr
            << "FAILED: "
            << message << '\n';

        std::exit(1);
    }
}

} // namespace

int main() {
    using namespace pipeframe::editor;

    SceneDocument document;
    SceneHistory history;

    const SceneObjectId objectId =
        document.CreateObject(
            "TEST OBJECT",
            "test.object");

    document.MarkClean();

    history.Record(document);

    SceneTransform movedTransform;
    movedTransform.position = {
        100.0f,
        50.0f,
    };

    Require(
        document.SetTransform(
            objectId,
            movedTransform),
        "Transform edit should succeed.");

    Require(
        history.CanUndo(),
        "Undo should be available.");

    Require(
        history.Undo(document),
        "Undo should succeed.");

    const SceneObjectData *object =
        document.FindObject(objectId);

    Require(
        object != nullptr,
        "Object should exist after undo.");

    Require(
        object->transform.position ==
            pipeframe::Vector2f{0.0f, 0.0f},
        "Undo should restore the original transform.");

    Require(
        history.CanRedo(),
        "Redo should be available.");

    Require(
        history.Redo(document),
        "Redo should succeed.");

    object = document.FindObject(objectId);

    Require(
        object != nullptr,
        "Object should exist after redo.");

    Require(
        object->transform.position ==
            pipeframe::Vector2f{100.0f, 50.0f},
        "Redo should restore the moved transform.");

    history.BeginContinuousEdit(document);

    SceneTransform draggedTransform =
        object->transform;

    draggedTransform.position = {
        300.0f,
        200.0f,
    };

    Require(
        document.SetTransform(
            objectId,
            draggedTransform),
        "Continuous transform should succeed.");

    Require(
        history.CancelContinuousEdit(document),
        "Continuous edit cancellation should succeed.");

    object = document.FindObject(objectId);

    Require(
        object->transform.position ==
            pipeframe::Vector2f{100.0f, 50.0f},
        "Cancel should restore the pre-drag document.");

    std::cout
        << "All scene history tests passed.\n";

    return 0;
}
