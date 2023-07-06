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

#ifndef ECMASCRIPT_COMPILER_TYPE_INFERENCE_INITIALIZATION_ANALYSIS_H
#define ECMASCRIPT_COMPILER_TYPE_INFERENCE_INITIALIZATION_ANALYSIS_H

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {
enum class ThisUsage : uint8_t {
    INDEFINITE_THIS = 0,
    DEFINITE_THIS,
};

class InitializationAnalysis {
public:
    InitializationAnalysis(Circuit *circuit, TSManager *tsManager, const CString &recordName,
                           const std::string &name, bool enableLog)
        : tsManager_(tsManager), circuit_(circuit), acc_(circuit), recordName_(recordName),
          methodName_(name), enableLog_(enableLog)
    {
        StoreThisObject();
    }
    ~InitializationAnalysis() = default;

    void Run();

private:
    inline bool IsLogEnabled() const
    {
        return enableLog_;
    }

    inline const std::string &GetMethodName() const
    {
        return methodName_;
    }

    inline bool IsThisFromArg(GateRef gate) const
    {
        return gate == thisObject_;
    }

    void Analyse(GateRef gate);
    void CollectInitializationInfo(GateRef gate, ThisUsage thisUsage);
    void CollectInitializationType(GateRef gate, ThisUsage thisUsage);
    bool CheckIsThisObject(GateRef receiver) const;
    void MarkSuperClass();
    bool CheckSimpleCFG(GateRef gate, const uint16_t index) const;
    bool CheckSimpleGate(GateRef gate, const uint16_t index) const;
    bool CheckSimpleJSGate(GateRef gate, const uint16_t index) const;
    bool CheckLdObjByName(GateRef gate, const uint16_t index, ThisUsage thisUsage) const;
    bool CheckLdObjByIndexOrValue(GateRef gate) const;
    bool CheckCall() const;
    void StoreThisObject();
    bool CheckThisAsValueIn(GateRef gate) const;
    void CollectThisEscapedInfo(GateRef gate);
    bool HasEscapedThis(GateRef gate) const;
    void MarkHasAnalysedInitialization();
    bool IsThisFromSuperCall(GateRef gate) const;
    void Print() const;

    TSManager *tsManager_ {nullptr};
    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    GateRef thisObject_ {Circuit::NullGate()};
    GateType classType_ {GateType::AnyType()};
    const CString &recordName_;
    const std::string &methodName_;
    bool enableLog_ {false};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TYPE_INFERENCE_INITIALIZATION_ANALYSIS_H
