#ifndef PIPEFRAME_SCENE_HISTORY_H
#define PIPEFRAME_SCENE_HISTORY_H

#include <cstddef>
#include <optional>
#include <vector>

#include "SceneDocument.h"

namespace pipeframe::editor {

class SceneHistory final {
public:
    explicit SceneHistory(std::size_t maximumEntries = 100);

    void Clear();

    void Record(const SceneDocument &document);

    bool Undo(SceneDocument &document);
    bool Redo(SceneDocument &document);

    bool CanUndo() const;
    bool CanRedo() const;

    void BeginContinuousEdit(const SceneDocument &document);
    void CommitContinuousEdit();
    bool CancelContinuousEdit(SceneDocument &document);

    bool HasContinuousEdit() const;

private:
    void LimitHistory(std::vector<SceneDocument> &history);

    std::size_t maximumEntries;

    std::vector<SceneDocument> undoHistory;
    std::vector<SceneDocument> redoHistory;

    std::optional<SceneDocument> continuousEditStart;
};

} // namespace pipeframe::editor

#endif