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

#ifndef ECMASCRIPT_JSOBJECT_RESIZING_STRATEGY_H
#define ECMASCRIPT_JSOBJECT_RESIZING_STRATEGY_H

namespace panda::ecmascript {
class JSThread;
class JSObjectResizingStrategy {
public:
    static constexpr int PROPERTIES_GROW_SIZE = 4;
    JSObjectResizingStrategy() = default;
    virtual ~JSObjectResizingStrategy() = default;
    virtual void UpdateGrowStep(JSThread *thread, uint32_t step = PROPERTIES_GROW_SIZE) = 0;
};

class ThroughputJSObjectResizingStrategy : public JSObjectResizingStrategy {
public:
    virtual void UpdateGrowStep(JSThread *thread, uint32_t step = PROPERTIES_GROW_SIZE) override;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSOBJECT_RESIZING_STRATEGY_H