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
#ifndef MAPLE_UTIL_INCLUDE_BASE_GRAPH_NODE_H
#define MAPLE_UTIL_INCLUDE_BASE_GRAPH_NODE_H
#include "types_def.h"
#include "mempool_allocator.h"
namespace maple {

template <typename T>
struct Comparator {
    bool operator()(const T *lhs, const T *rhs) const
    {
        return lhs->GetID() < rhs->GetID();
    }
};

class BaseGraphNode {
public:
    virtual void GetOutNodes(std::vector<BaseGraphNode *> &outNodes) = 0;
    virtual void GetOutNodes(std::vector<BaseGraphNode *> &outNodes) const = 0;
    virtual void GetInNodes(std::vector<BaseGraphNode *> &outNodes) = 0;
    virtual void GetInNodes(std::vector<BaseGraphNode *> &outNodes) const = 0;
    virtual const std::string GetIdentity() = 0;
    virtual uint32 GetID() const = 0;
};

// BaseGraphNode needs to be the base class of T
template <typename T, std::enable_if_t<std::is_base_of<BaseGraphNode, T>::value, bool> = true>
void ConvertToVectorOfBasePtr(const MapleVector<T *> &originVec, MapleVector<BaseGraphNode *> &targetVec)
{
    for (auto &item : originVec) {
        targetVec.emplace_back(item);
    }
}
}  // namespace maple
#endif /* MAPLE_UTIL_INCLUDE_BASE_GRAPH_NODE_H */
