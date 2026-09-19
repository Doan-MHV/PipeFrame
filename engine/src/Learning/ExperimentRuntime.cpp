#include <PipeFrame/Learning/ExperimentRuntime.h>

#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>

namespace pipeframe::learning {

ExperimentClock::ExperimentClock(const float limitSeconds) { SetLimit(limitSeconds); }
void ExperimentClock::Reset() { elapsed = 0.0f; }
void ExperimentClock::Advance(const float deltaTime) {
    if (std::isfinite(deltaTime) && deltaTime > 0.0f)
        elapsed += deltaTime;
}
void ExperimentClock::SetLimit(const float seconds) { limit = std::isfinite(seconds) ? std::max(0.0f, seconds) : 0.0f; }
float ExperimentClock::Elapsed() const { return elapsed; }
float ExperimentClock::Limit() const { return limit; }
float ExperimentClock::Progress() const { return limit > 0.0f ? std::clamp(elapsed / limit, 0.0f, 1.0f) : 0.0f; }
bool ExperimentClock::Expired() const { return limit > 0.0f && elapsed >= limit; }

double ExperimentHistorySample::Metric(const std::string_view name, const double fallback) const {
    const auto found = metrics.find(std::string(name));
    return found == metrics.end() ? fallback : found->second;
}

void ExperimentHistory::Add(ExperimentHistorySample sample) { samples.push_back(std::move(sample)); }
void ExperimentHistory::Clear() { samples.clear(); }
std::span<const ExperimentHistorySample> ExperimentHistory::Samples() const { return samples; }

bool ExperimentHistory::SaveCsv(const std::filesystem::path &path, std::string &errorMessage) const {
    errorMessage.clear();
    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        errorMessage = "Unable to create experiment history: " + path.string();
        return false;
    }
    output << "run,iteration,best_score,average_score,progress,node_count,connection_count,metrics\n";
    for (const ExperimentHistorySample &sample : samples) {
        output << sample.run << ',' << sample.iteration << ',' << sample.bestScore << ',' << sample.averageScore << ','
               << sample.progress << ',' << sample.modelNodeCount << ',' << sample.modelConnectionCount << ',';
        bool first = true;
        for (const auto &[name, value] : sample.metrics) {
            if (!first)
                output << ';';
            first = false;
            output << name << '=' << value;
        }
        output << '\n';
    }
    if (!output.good()) {
        errorMessage = "Unable to write experiment history: " + path.string();
        return false;
    }
    return true;
}

bool ExperimentHistory::LoadCsv(const std::filesystem::path &path, std::string &errorMessage) {
    errorMessage.clear();
    std::ifstream input(path);
    if (!input) {
        errorMessage = "Unable to open experiment history: " + path.string();
        return false;
    }
    std::string line;
    if (!std::getline(input, line)) {
        errorMessage = "Experiment history is empty.";
        return false;
    }
    std::vector<ExperimentHistorySample> loaded;
    while (std::getline(input, line)) {
        if (line.empty())
            continue;
        std::istringstream row(line);
        ExperimentHistorySample sample;
        char separator{};
        if (!(row >> sample.run >> separator) || separator != ',' || !(row >> sample.iteration >> separator) ||
            separator != ',' || !(row >> sample.bestScore >> separator) || separator != ',' ||
            !(row >> sample.averageScore >> separator) || separator != ',' || !(row >> sample.progress >> separator) ||
            separator != ',' || !(row >> sample.modelNodeCount >> separator) || separator != ',' ||
            !(row >> sample.modelConnectionCount >> separator) || separator != ',') {
            errorMessage = "Invalid experiment history row: " + line;
            return false;
        }
        std::string encoded;
        std::getline(row, encoded);
        std::istringstream metrics(encoded);
        std::string item;
        while (std::getline(metrics, item, ';')) {
            const std::size_t equals = item.find('=');
            if (equals == std::string::npos)
                continue;
            try {
                sample.metrics[item.substr(0, equals)] = std::stod(item.substr(equals + 1));
            } catch (...) {
                errorMessage = "Invalid experiment metric: " + item;
                return false;
            }
        }
        loaded.push_back(std::move(sample));
    }
    samples = std::move(loaded);
    return true;
}

bool CheckpointMetadata::Save(const std::filesystem::path &path, std::string &errorMessage) const {
    errorMessage.clear();
    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        errorMessage = "Unable to create checkpoint metadata: " + path.string();
        return false;
    }
    output << "PIPEFRAME_EXPERIMENT " << version << '\n'
           << trainerId << '\n'
           << run << ' ' << iteration << ' ' << seed << ' ' << population << '\n';
    for (const auto &[key, value] : values)
        output << key << '=' << value << '\n';
    if (!output.good()) {
        errorMessage = "Unable to write checkpoint metadata: " + path.string();
        return false;
    }
    return true;
}

bool CheckpointMetadata::Load(const std::filesystem::path &path, std::string &errorMessage) {
    errorMessage.clear();
    std::ifstream input(path);
    std::string magic;
    CheckpointMetadata loaded;
    if (!(input >> magic >> loaded.version) || magic != "PIPEFRAME_EXPERIMENT" || loaded.version != 1) {
        errorMessage = "Invalid or unsupported experiment checkpoint metadata.";
        return false;
    }
    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (!std::getline(input, loaded.trainerId) ||
        !(input >> loaded.run >> loaded.iteration >> loaded.seed >> loaded.population)) {
        errorMessage = "Incomplete experiment checkpoint metadata.";
        return false;
    }
    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::string line;
    while (std::getline(input, line)) {
        const std::size_t equals = line.find('=');
        if (equals != std::string::npos)
            loaded.values[line.substr(0, equals)] = line.substr(equals + 1);
    }
    *this = std::move(loaded);
    return true;
}

} // namespace pipeframe::learning
