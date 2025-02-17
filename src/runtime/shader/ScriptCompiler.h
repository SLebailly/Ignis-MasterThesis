#pragma once

#include "IG_Config.h"

#include <mutex>

namespace IG {
class IG_LIB ScriptCompiler {
public:
    ScriptCompiler();
    ~ScriptCompiler();

    inline void setOptimizationLevel(size_t l) { mOptimizationLevel = l; }
    inline size_t optimizationLevel() const { return mOptimizationLevel; }

    inline void setVerbose(bool verbose) { mVerbose = verbose; }
    inline bool isVerbose() const { return mVerbose; }

    std::string prepare(const std::string& script) const;
    void* compile(const std::string& script, const std::string& function) const;
    void loadStdLibFromDirectory(const Path& dir);

private:
    std::string mStdLibOverride;
    size_t mOptimizationLevel;
    bool mVerbose;

    mutable std::mutex mCompileMutex;
};
} // namespace IG