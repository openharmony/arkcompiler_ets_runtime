/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "itab_util.h"
#include <map>
#include <mutex>
#include <cstring>

namespace maple {
unsigned int DJBHash(const char *str)
{
    unsigned int hash = 5381;  // initial value for DJB hash algorithm
    while (*str) {
        hash += (hash << 5) + (unsigned char)(*str++);  // calculate the hash code of data
    }
    return (hash & 0x7FFFFFFF);
}

unsigned int GetHashIndex(const char *name)
{
    unsigned int hashcode = DJBHash(name);
    return (hashcode % kHashSize);
}

struct CmpStr {
    bool operator()(char const *a, char const *b) const
    {
        return std::strcmp(a, b) < 0;
    }
};

// optimization for hot method retrival.
// check risk incurred by multi-threads.
std::map<const char *, unsigned int, CmpStr> hotMethodCache;
std::map<const char *, unsigned int>::iterator it;
std::mutex mapLock;

unsigned int GetSecondHashIndex(const char *name)
{
    std::lock_guard<std::mutex> guard(mapLock);
    if (hotMethodCache.size() == 0) {
        for (unsigned int i = 0; i < ITAB_HOTMETHOD_SIZE; ++i) {
            hotMethodCache.emplace(itabHotMethod[i], i);
        }
    }

    it = hotMethodCache.find(name);
    if (it != hotMethodCache.end()) {
        return it->second;
    }
    unsigned int hashcode = DJBHash(name);
    return ITAB_HOTMETHOD_SIZE + (hashcode % (kItabSecondHashSize - ITAB_HOTMETHOD_SIZE));
}
}  // namespace maple
