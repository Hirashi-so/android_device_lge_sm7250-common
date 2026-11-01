#pragma once

#include <aidl/android/hardware/radio/config/SimSlotStatus.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

namespace android::hardware::radio::compat {

struct SlotMappingEntry {
    int32_t physicalSlotId;
    int32_t logicalSlotId;
    bool active;
};

struct SlotMappingSnapshot {
    bool valid = false;
    size_t physicalSlotCount = 0;
    std::vector<SlotMappingEntry> entries;
    std::array<int32_t, 2> presentSlots{{-1, -1}};
};

class SlotMappingState final {
    mutable std::mutex mMutex;
    SlotMappingSnapshot mSnapshot;
    std::array<int32_t, 2> mPresentSlots{{-1, -1}};

    SlotMappingState() = default;

  public:
    static SlotMappingState& get();

    void update(const std::vector<
                ::aidl::android::hardware::radio::config::SimSlotStatus>& statuses);

    SlotMappingSnapshot snapshot() const;
    SlotMappingSnapshot observeCardStatus(
            int32_t phoneId, bool present, int32_t physicalSlotId);
};

}
