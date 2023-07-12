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

#ifndef ECMASCRIPT_COMPILER_TS_HCLASS_GENERATOR_H
#define ECMASCRIPT_COMPILER_TS_HCLASS_GENERATOR_H

#include "ecmascript/pgo_profiler/pgo_profiler_layout.h"
#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {
class TSHClassGenerator {
public:
    explicit TSHClassGenerator(TSManager *tsManager): tsManager_(tsManager) {};
    ~TSHClassGenerator() = default;

    void GenerateTSHClasses() const;
    void UpdateTSHClassFromPGO(const kungfu::GateType &type, const PGOHClassLayoutDesc &desc,
        bool enableOptTrackField) const;

private:
    void RecursiveGenerate(const JSHandle<TSClassType> &classType) const;

    JSHandle<JSHClass> Generate(const JSHandle<TSClassType> &classType) const;

    enum class Kind : uint8_t {
        INSTANCE = 0,
        PROTOTYPE,
        CONSTRUCTOR,
    };

    JSHandle<JSHClass> CreateHClass(const JSThread *thread, const JSHandle<TSClassType> &classType, Kind kind) const;

    JSHandle<JSHClass> CreateIHClass(const JSThread *thread, const JSHandle<TSClassType> &classType) const;

    JSHandle<JSHClass> CreatePHClass(const JSThread *thread, const JSHandle<TSClassType> &classType) const;

    JSHandle<JSHClass> CreateCHClass(const JSThread *thread, const JSHandle<TSClassType> &classType) const;

    inline const std::set<GlobalTSTypeRef>& GetCollectedGT() const
    {
        return tsManager_->GetCollectedGT();
    }

    TSManager *tsManager_ {nullptr};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TS_HCLASS_GENERATOR_H
