#include <libradiocompat/SlotMappingState.h>

#include <utility>

namespace android::hardware::radio::compat {

SlotMappingState& SlotMappingState::get() {
    static SlotMappingState instance;
    return instance;
}

void SlotMappingState::update(
        const std::vector<
                ::aidl::android::hardware::radio::config::SimSlotStatus>& statuses) {
    SlotMappingSnapshot next;
    next.valid = !statuses.empty();
    next.physicalSlotCount = statuses.size();

    for (size_t physical = 0; physical < statuses.size(); ++physical) {
        for (const auto& port : statuses[physical].portInfo) {
            next.entries.push_back({
                    static_cast<int32_t>(physical),
                    port.logicalSlotId,
                    port.portActive,
            });
        }
    }

    std::lock_guard<std::mutex> lock(mMutex);
    mSnapshot = std::move(next);
}

SlotMappingSnapshot SlotMappingState::snapshot() const {
    std::lock_guard<std::mutex> lock(mMutex);
    auto result = mSnapshot;
    result.presentSlots = mPresentSlots;
    return result;
}

SlotMappingSnapshot SlotMappingState::observeCardStatus(
        int32_t phoneId, bool present, int32_t physicalSlotId) {
    std::lock_guard<std::mutex> lock(mMutex);

    if (phoneId >= 0 &&
        phoneId < static_cast<int32_t>(mPresentSlots.size())) {
        mPresentSlots[phoneId] = present ? physicalSlotId : -1;
    }

    auto result = mSnapshot;
    result.presentSlots = mPresentSlots;
    return result;
}

}
