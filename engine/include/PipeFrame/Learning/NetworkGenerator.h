#ifndef PIPEFRAME_LEARNING_NETWORK_GENERATOR_H
#define PIPEFRAME_LEARNING_NETWORK_GENERATOR_H

#include <optional>
#include <string>

#include <PipeFrame/Learning/Genome.h>
#include <PipeFrame/Learning/Network.h>

namespace pipeframe::learning {

class NetworkGenerator final {
public:
    [[nodiscard]] static std::optional<Network> Generate(const Genome &genome,
                                                         std::string *errorMessage = nullptr);
};

} // namespace pipeframe::learning

#endif

