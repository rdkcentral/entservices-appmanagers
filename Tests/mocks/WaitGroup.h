/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

class WaitGroup {
public:
    WaitGroup()
        : _count(0)
    {
    }

    void Add(int count = 1)
    {
         std::lock_guard<std::mutex> lock(_m);
        _count += count;

        if (_count <= 0)
            _cv.notify_all();
    }

    void Done() { Add(-1); }

    void Wait()
    {
        if (_count <= 0)
            return;

        std::unique_lock<std::mutex> _lock{ _m };
        _cv.wait(_lock, [&]() {
            return _count == 0;
        });
    }

    // avoid copies of this class
    WaitGroup(const WaitGroup&) = delete;
    WaitGroup& operator=(const WaitGroup&) = delete;

private:
    std::atomic<int> _count;
    std::mutex _m;
    std::condition_variable _cv;
};
