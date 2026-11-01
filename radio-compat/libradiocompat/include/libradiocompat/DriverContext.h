/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>

#include <aidl/android/hardware/radio/data/DataProfileInfo.h>

#include <cstdint>
#include <mutex>
#include <unordered_set>
#include <map>
#include <string>

namespace android::hardware::radio::compat {

class DriverContext {
    struct SignalCriteriaPending {
        size_t remaining;
        int32_t error = 0;
        bool ackForwarded = false;
        std::function<void()> acknowledge;
    };

    std::mutex mSignalCriteriaMutex;
    std::unordered_map<int32_t, SignalCriteriaPending> mSignalCriteriaPending;

    std::mutex mImeiMutex;
    std::unordered_set<int32_t> mPendingImeiRequests;
    std::map<std::string, ::aidl::android::hardware::radio::data::DataProfileInfo> mDataProfiles;

    std::string mSlotName;

  public:
    struct SignalCriteriaCompletion {
        bool forward;
        int32_t error;
        std::function<void()> acknowledge;
    };

    void beginSignalCriteria(int32_t serial, size_t count,
                             std::function<void()> acknowledge) {
        std::lock_guard<std::mutex> lock(mSignalCriteriaMutex);
        mSignalCriteriaPending[serial] = {count, 0, false, std::move(acknowledge)};
    }

    SignalCriteriaCompletion completeSignalCriteria(int32_t serial, int32_t error) {
        std::lock_guard<std::mutex> lock(mSignalCriteriaMutex);
        auto it = mSignalCriteriaPending.find(serial);
        if (it == mSignalCriteriaPending.end()) {
            return {true, error, {}};
        }

        auto& pending = it->second;
        if (pending.error == 0 && error != 0) {
            pending.error = error;
        }

        if (--pending.remaining != 0) {
            return {false, pending.error, pending.acknowledge};
        }

        const int32_t finalError = pending.error;
        mSignalCriteriaPending.erase(it);
        return {true, finalError, {}};
    }

    bool shouldForwardSignalCriteriaAck(int32_t serial) {
        std::lock_guard<std::mutex> lock(mSignalCriteriaMutex);
        auto it = mSignalCriteriaPending.find(serial);
        if (it == mSignalCriteriaPending.end()) {
            return true;
        }
        if (it->second.ackForwarded) {
            return false;
        }
        it->second.ackForwarded = true;
        return true;
    }

    void setSlotName(const std::string& slot) { mSlotName = slot; }
    const std::string& slotName() const { return mSlotName; }

    bool addImeiRequest(int32_t serial) {
        std::lock_guard<std::mutex> lock(mImeiMutex);
        return mPendingImeiRequests.insert(serial).second;
    }

    bool takeImeiRequest(int32_t serial) {
        std::lock_guard<std::mutex> lock(mImeiMutex);
        return mPendingImeiRequests.erase(serial) != 0;
    }

    void addDataProfile(const ::aidl::android::hardware::radio::data::DataProfileInfo& profile);
    ::aidl::android::hardware::radio::data::DataProfileInfo getDataProfile(const std::string& apn);
};

}  // namespace android::hardware::radio::compat
