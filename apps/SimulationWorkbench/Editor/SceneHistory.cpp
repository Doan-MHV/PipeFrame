#include "SceneHistory.h"

#include <utility>

namespace pipeframe::editor {

SceneHistory::SceneHistory(
    const std::size_t maximumEntries)
    : maximumEntries(maximumEntries) {}

void SceneHistory::Clear() {
    undoHistory.clear();
    redoHistory.clear();
    continuousEditStart.reset();
}

void SceneHistory::Record(
    const SceneDocument &document) {

    if (continuousEditStart.has_value()) {
        return;
    }

    undoHistory.push_back(document);
    LimitHistory(undoHistory);

    redoHistory.clear();
}

bool SceneHistory::Undo(
    SceneDocument &document) {

    if (undoHistory.empty() ||
        continuousEditStart.has_value()) {

        return false;
    }

    redoHistory.push_back(document);
    LimitHistory(redoHistory);

    document = std::move(undoHistory.back());
    undoHistory.pop_back();

    return true;
}

bool SceneHistory::Redo(
    SceneDocument &document) {

    if (redoHistory.empty() ||
        continuousEditStart.has_value()) {

        return false;
    }

    undoHistory.push_back(document);
    LimitHistory(undoHistory);

    document = std::move(redoHistory.back());
    redoHistory.pop_back();

    return true;
}

bool SceneHistory::CanUndo() const {
    return !undoHistory.empty() &&
           !continuousEditStart.has_value();
}

bool SceneHistory::CanRedo() const {
    return !redoHistory.empty() &&
           !continuousEditStart.has_value();
}

void SceneHistory::BeginContinuousEdit(
    const SceneDocument &document) {

    if (continuousEditStart.has_value()) {
        return;
    }

    continuousEditStart = document;
}

void SceneHistory::CommitContinuousEdit() {
    if (!continuousEditStart.has_value()) {
        return;
    }

    undoHistory.push_back(
        std::move(*continuousEditStart));

    continuousEditStart.reset();

    LimitHistory(undoHistory);
    redoHistory.clear();
}

bool SceneHistory::CancelContinuousEdit(
    SceneDocument &document) {

    if (!continuousEditStart.has_value()) {
        return false;
    }

    document =
        std::move(*continuousEditStart);

    continuousEditStart.reset();

    return true;
}

bool SceneHistory::HasContinuousEdit() const {
    return continuousEditStart.has_value();
}

void SceneHistory::LimitHistory(
    std::vector<SceneDocument> &history) {

    while (history.size() > maximumEntries) {
        history.erase(history.begin());
    }
}

} // namespace pipeframe::editor