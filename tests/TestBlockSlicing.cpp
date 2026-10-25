/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for Sample-Accurate Block Slicing Engine
 */

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>
#include <algorithm>

struct SliceLog {
    uint32_t startFrame;
    uint32_t numFrames;
    float paramValue;
};

struct QueuedParamChange {
    uint32_t offset;
    uint32_t index;
    float value;
};

// Simulation of the DPF block-slicing algorithm used in VST3 and CLAP
template <typename ProcessSliceFunc>
void executeBlockSlicing(const uint32_t totalFrames,
                         QueuedParamChange* const paramChanges,
                         const uint32_t paramChangeCount,
                         float& activeParamValue,
                         ProcessSliceFunc&& processSlice)
{
    if (paramChangeCount > 1)
    {
        std::sort(paramChanges, paramChanges + paramChangeCount,
                  [](const QueuedParamChange& a, const QueuedParamChange& b) noexcept {
                      return a.offset < b.offset;
                  });
    }

    uint32_t currentFrame = 0;
    uint32_t paramIdx = 0;
    uint32_t sliceCount = 0;
    static constexpr const uint32_t kMaxSlices = 32;

    while (currentFrame < totalFrames)
    {
        while (paramIdx < paramChangeCount && paramChanges[paramIdx].offset <= currentFrame)
        {
            activeParamValue = paramChanges[paramIdx].value;
            ++paramIdx;
        }

        uint32_t nextFrame = totalFrames;
        if (sliceCount + 1 < kMaxSlices && paramIdx < paramChangeCount)
        {
            nextFrame = paramChanges[paramIdx].offset;
            if (nextFrame <= currentFrame)
                nextFrame = currentFrame + 1;
            if (nextFrame > totalFrames)
                nextFrame = totalFrames;
        }

        const uint32_t sliceFrames = nextFrame - currentFrame;
        if (sliceFrames == 0)
            break;

        processSlice(currentFrame, sliceFrames, activeParamValue);

        currentFrame = nextFrame;
        ++sliceCount;
    }

    while (paramIdx < paramChangeCount)
    {
        activeParamValue = paramChanges[paramIdx].value;
        ++paramIdx;
    }
}

void testNoParamChanges() {
    float param = 0.5f;
    std::vector<SliceLog> log;

    executeBlockSlicing(128, nullptr, 0, param, [&](uint32_t start, uint32_t frames, float val) {
        log.push_back({ start, frames, val });
    });

    assert(log.size() == 1);
    assert(log[0].startFrame == 0);
    assert(log[0].numFrames == 128);
    assert(log[0].paramValue == 0.5f);
    std::cout << "[PASS] testNoParamChanges (Single pass, zero slicing overhead)\n";
}

void testIntraBlockChanges() {
    float param = 0.0f;
    std::vector<SliceLog> log;

    QueuedParamChange changes[] = {
        { 0,  0, 0.1f },
        { 32, 0, 0.5f },
        { 64, 0, 0.9f }
    };

    executeBlockSlicing(128, changes, 3, param, [&](uint32_t start, uint32_t frames, float val) {
        log.push_back({ start, frames, val });
    });

    assert(log.size() == 3);
    // Slice 1: frames 0..32, param = 0.1
    assert(log[0].startFrame == 0);
    assert(log[0].numFrames == 32);
    assert(std::fabs(log[0].paramValue - 0.1f) < 1e-6f);

    // Slice 2: frames 32..64, param = 0.5
    assert(log[1].startFrame == 32);
    assert(log[1].numFrames == 32);
    assert(std::fabs(log[1].paramValue - 0.5f) < 1e-6f);

    // Slice 3: frames 64..128, param = 0.9
    assert(log[2].startFrame == 64);
    assert(log[2].numFrames == 64);
    assert(std::fabs(log[2].paramValue - 0.9f) < 1e-6f);

    uint32_t totalProcessed = 0;
    for (const auto& s : log)
        totalProcessed += s.numFrames;
    assert(totalProcessed == 128);

    std::cout << "[PASS] testIntraBlockChanges (Sample-accurate slicing at frames 0, 32, 64)\n";
}

void testUnsortedAndDuplicateOffsets() {
    float param = 0.0f;
    std::vector<SliceLog> log;

    // Out of order and duplicate offsets
    QueuedParamChange changes[] = {
        { 64, 0, 0.8f },
        { 16, 0, 0.2f },
        { 16, 1, 0.3f }, // duplicate offset
        { 0,  0, 0.0f }
    };

    executeBlockSlicing(128, changes, 4, param, [&](uint32_t start, uint32_t frames, float val) {
        log.push_back({ start, frames, val });
    });

    // Should slice at 0..16, 16..64, 64..128 (duplicate at 16 applied together)
    assert(log.size() == 3);
    assert(log[0].startFrame == 0 && log[0].numFrames == 16);
    assert(log[1].startFrame == 16 && log[1].numFrames == 48);
    assert(log[2].startFrame == 64 && log[2].numFrames == 64);

    uint32_t totalProcessed = 0;
    for (const auto& s : log)
        totalProcessed += s.numFrames;
    assert(totalProcessed == 128);

    std::cout << "[PASS] testUnsortedAndDuplicateOffsets (Auto-sorted, duplicate offsets coalesced)\n";
}

void testMaxSlicesSafetyGuard() {
    float param = 0.0f;
    std::vector<SliceLog> log;

    // 100 changes in a 256 frame buffer (pathological flooding)
    QueuedParamChange changes[100];
    for (uint32_t i = 0; i < 100; ++i) {
        changes[i] = { i * 2, 0, static_cast<float>(i) };
    }

    executeBlockSlicing(256, changes, 100, param, [&](uint32_t start, uint32_t frames, float val) {
        log.push_back({ start, frames, val });
    });

    // Must be bounded by kMaxSlices (32)
    assert(log.size() <= 32);

    uint32_t totalProcessed = 0;
    for (const auto& s : log)
        totalProcessed += s.numFrames;
    assert(totalProcessed == 256); // All frames still processed without sample loss!
    assert(param == 99.0f); // Final parameter value reached accurately!

    std::cout << "[PASS] testMaxSlicesSafetyGuard (Safely capped at " << log.size() << " slices, exactly 256 frames processed)\n";
}

int main() {
    std::cout << "Running Block-Slicing Engine Tests...\n";
    testNoParamChanges();
    testIntraBlockChanges();
    testUnsortedAndDuplicateOffsets();
    testMaxSlicesSafetyGuard();
    std::cout << "All Block-Slicing Tests Passed Successfully!\n";
    return 0;
}
