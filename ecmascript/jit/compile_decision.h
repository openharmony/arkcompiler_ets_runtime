/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JIT_COMPILE_DECISION_H
#define ECMASCRIPT_JIT_COMPILE_DECISION_H

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript {
class JitCompileMode {
public:
    enum class Mode : uint8_t {
        SYNC = 0,
        ASYNC
    };

    JitCompileMode(Mode mode) : mode_(mode) {}

    bool IsAsync() const
    {
        return mode_ == Mode::ASYNC;
    }

    bool IsSync() const
    {
        return mode_ == Mode::SYNC;
    }

private:
    Mode mode_;
};

class CompilerTier {
public:
    enum class Tier : uint8_t {
        BASELINE,
        FAST,
    };

    CompilerTier(Tier tier) : tier_(tier) {}
    bool IsFast() const
    {
        return tier_ == Tier::FAST;
    }

    bool IsBaseLine() const
    {
        return tier_ == Tier::BASELINE;
    }

    friend std::ostream &operator<<(std::ostream &os, const CompilerTier &tier)
    {
        os << (tier.IsFast() ? "[fastjit] " : " [baselinejit] ");
        return os;
    }

private:
    Tier tier_;
};

class MethodNameCollector {
public:
    void Init(EcmaVM *vm);
    inline void Collect(const std::string& methodFullName) const;
    ~MethodNameCollector();
private:
    mutable std::ofstream file_;
    bool enable_ {false};
    bool isInit_ {false};
};

class MethodNameFilter {
public:
    void Init(EcmaVM *vm);
    inline bool NeedCompiledByJit(const std::string& methodFullName) const;
    ~MethodNameFilter();
private:
    std::set<std::string> methodFullNames;
    bool enable_ {false};
    bool isInit_ {false};
};

class CompileDecision {
public:
    CompileDecision(EcmaVM *vm, JSHandle<JSFunction> &jsFunction, CompilerTier tier,
        int32_t osrOffset, JitCompileMode mode);
    bool Decision();

    CompilerTier GetTier() const
    {
        return tier_;
    }

    JSHandle<JSFunction> GetJsFunction() const
    {
        return jsFunction_;
    }

    CString GetMethodInfo() const;
    CString GetMethodName() const;
    uint32_t GetCodeSize() const;
    int32_t GetOsrOffset() const
    {
        return osrOffset_;
    }

    JitCompileMode GetCompileMode() const
    {
        return compileMode_;
    }

    static inline MethodNameCollector &GetMethodNameCollector()
    {
        return methodNameCollector;
    }

    static inline MethodNameFilter &GetMethodNameFilter()
    {
        return methodNameFilter;
    }

private:
    bool IsGoodCompilationRequest() const;
    bool IsSupportFunctionKind() const;
    bool CheckJsFunctionStatus() const;
    bool IsJsFunctionSupportCompile() const;
    void DisableJitCompile() const;
    bool CheckVmState() const;

    EcmaVM *vm_;
    JSHandle<JSFunction> jsFunction_;
    CompilerTier tier_;
    int32_t osrOffset_;
    JitCompileMode compileMode_;
    static MethodNameCollector methodNameCollector;
    static MethodNameFilter methodNameFilter;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JIT_JIT_COMPILE_DECISION_H
