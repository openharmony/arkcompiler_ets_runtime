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

#ifndef ECMASCRIPT_COMPILER_CALL_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_CALL_STUB_BUILDER_H

#include <cstddef>
#include "ecmascript/compiler/profiler_operation.h"
#include "ecmascript/compiler/stub_builder.h"

namespace panda::ecmascript::kungfu {

struct CallArgs {
    GateRef arg0;
    GateRef arg1;
    GateRef arg2;
};

struct CallArgsWithThis {
    GateRef arg0;
    GateRef arg1;
    GateRef arg2;
    GateRef thisValue;
};

struct CallArgv {
    GateRef argc;
    GateRef argv;
};

struct CallArgvWithThis {
    GateRef argc;
    GateRef argv;
    GateRef thisValue;
};

struct SuperCallArgs {
    GateRef thisFunc;
    GateRef array;
    GateRef argc;
    GateRef argv;
    GateRef thisObj;
    GateRef newTarget;
};

struct CallConstructorArgs {
    GateRef argc;
    GateRef argv;
    GateRef thisObj;
};

struct CallGetterArgs {
    GateRef receiver;
};

struct CallSetterArgs {
    GateRef receiver;
    GateRef value;
};

struct CallThisArg2WithReturnArgs {
    GateRef thisValue;
    GateRef arg0;
    GateRef arg1;
};

struct CallThisArg3WithReturnArgs {
    GateRef argHandle;
    GateRef value;
    GateRef key;
    GateRef thisValue;
};

struct CallThisArgvWithReturnArgs {
    GateRef argc;
    GateRef argv;
    GateRef thisValue;
};

struct JSCallArgs {
    JSCallArgs() {}
    JSCallArgs(JSCallMode m) : mode(m) {}
    JSCallMode mode {JSCallMode::CALL_ARG0};
    union {
        CallArgs callArgs;
        CallArgsWithThis callArgsWithThis;
        CallArgv callArgv;
        CallArgvWithThis callArgvWithThis;
        SuperCallArgs superCallArgs;
        CallConstructorArgs callConstructorArgs;
        CallGetterArgs callGetterArgs;
        CallSetterArgs callSetterArgs;
        CallThisArg2WithReturnArgs callThisArg2WithReturnArgs;
        CallThisArg3WithReturnArgs callThisArg3WithReturnArgs;
        CallThisArgvWithReturnArgs callThisArgvWithReturnArgs;
    };
};

class CallCoStubBuilder : public StubBuilder {
public:
    explicit CallCoStubBuilder(StubBuilder *parent, EcmaOpcode op)
        : StubBuilder(parent)
    {
        this->op_ = op;
    }
    ~CallCoStubBuilder() override = default;
    NO_MOVE_SEMANTIC(CallCoStubBuilder);
    NO_COPY_SEMANTIC(CallCoStubBuilder);
    void GenerateCircuit() override {}
    void PrepareArgs(std::vector<GateRef> &args, std::vector<GateRef> &argsFastCall);
    GateRef CallStubDispatch();
    std::tuple<bool, size_t, size_t> GetOpInfo();
    static void LowerFastCall(GateRef gate, GateRef glue, CircuitBuilder &builder, GateRef func, GateRef argc,
                              const std::vector<GateRef> &args, const std::vector<GateRef> &fastCallArgs,
                              Variable *result, Label *exit, bool isNew);
    static void FastCallSelector(CircuitBuilder &builder, GateRef glue, GateRef func, GateRef argc, Variable *result,
                                 Label *exit);
    static void LowerFastSuperCall(GateRef glue, CircuitBuilder &builder, const std::vector<GateRef> &args,
                                   GateRef elementsPtr, Variable &result, Label &exit);

    static GateRef LowerCallNGCRuntime(GateRef glue, CircuitBuilder &builder, GateRef gate, int index,
                                       const std::vector<GateRef> &args, bool useLabel = false);
    static void CallNGCRuntimeWithCallTimer(GateRef glue, CircuitBuilder &builder, int index, GateRef gate,
                                            GateRef func, Variable &result, const std::vector<GateRef> &args);

private:
    GateRef glue_ { Circuit::NullGate() };
    GateRef actualArgc_ { Circuit::NullGate() };
    GateRef actualArgv_ { Circuit::NullGate() };
    GateRef func_ { Circuit::NullGate() };
    GateRef newTarget_ { Circuit::NullGate() };
    GateRef hirGate_ { Circuit::NullGate() };
    EcmaOpcode op_;
    GateRef thisObj_ { Circuit::NullGate() };
};

enum class FastCallType {
    FAST_AOT_CALL,
    FAST_AOT_CALL_BRIDGE,
    AOT_CALL,
    AOT_CALL_BRIDGE,
    SLOW_CALL
};

class CallStubBuilder : public StubBuilder {
public:
    explicit CallStubBuilder(StubBuilder *parent, GateRef glue, GateRef func, GateRef actualNumArgs, GateRef jumpSize,
                             Variable *result, GateRef hotnessCounter, JSCallArgs callArgs,
                             ProfileOperation callback = ProfileOperation(),
                             bool checkIsCallable = true, GateRef hir = Circuit::NullGate())
        : StubBuilder(parent)
    {
        this->glue_ = glue;
        this->func_ = func;
        this->jumpSize_ = jumpSize;
        this->actualNumArgs_ = actualNumArgs;
        this->result_ = result;
        this->hotnessCounter_ = hotnessCounter;
        this->callArgs_ = callArgs;
        this->callback_ = callback;
        this->checkIsCallable_ = checkIsCallable;
        this->hir_ = hir;
    }
    explicit CallStubBuilder(Environment *env)
        : StubBuilder(env) {}
    ~CallStubBuilder() override = default;
    NO_MOVE_SEMANTIC(CallStubBuilder);
    NO_COPY_SEMANTIC(CallStubBuilder);
    void GenerateCircuit() override {}

    void JSCallDispatchForBaseline(Label *exit, Label *noNeedCheckException = nullptr);
    GateRef JSCallDispatch();

protected:

private:
    GateRef glue_ {0};
    GateRef func_ {0};
    GateRef jumpSize_ {0};
    GateRef actualNumArgs_ {0};
    GateRef hotnessCounter_ {0};
    Variable *result_ {nullptr};
    JSCallArgs callArgs_;
    ProfileOperation callback_;
    bool checkIsCallable_ {true};
    GateRef hir_ {0};

    bool isFast_ {true};
    bool isBridge_ {false};
    bool isForBaseline_ {false};
    GateRef sp_ {0};
    GateRef method_ {0};
    GateRef numArgs_ {0};
    GateRef bitfield_ {0};
    GateRef callField_ {0};
    GateRef newTarget_ {0};
    GateRef thisValue_ {0};
    GateRef nativeCode_ {0};
    GateRef realNumArgs_ {0};
    GateRef isNativeMask_ {0};
    GateRef baselineBuiltinFp_ {0};

    void JSCallInit(Label *exit, Label *funcIsHeapObject, Label *funcIsCallable, Label *funcNotCallable);
    void JSCallNative(Label *exit);
    void JSCallNativeInner(Label *exit, bool isJsProxy);
    void JSCallJSFunction(Label *exit, Label *noNeedCheckException = nullptr);
    void JSFastAotCall(Label *exit);
    void JSSlowAotCall(Label *exit);
    GateRef CallConstructorBridge(const int idxForAot, const std::vector<GateRef> &argsForAot);
    void CallBridge(GateRef code, GateRef expectedNum, Label *exit);
    void JSCallAsmInterpreter(bool hasBaselineCode, Label *exit, Label *noNeedCheckException);

    int PrepareIdxForNative();
    std::vector<GateRef> PrepareArgsForNative();
    std::vector<GateRef> PrepareBasicArgsForNative();
    std::vector<GateRef> PrepareAppendArgsForNative();

    int PrepareIdxForAot();
    std::vector<GateRef> PrepareArgsForAot(GateRef expectedNum);
    std::vector<GateRef> PrepareBasicArgsForAot();
    std::vector<GateRef> PrepareAppendArgsForAotStep1();
    std::vector<GateRef> PrepareAppendArgsForAotStep2();
    std::vector<GateRef> PrepareAppendArgsForAotStep3(GateRef expectedNum);

    int PrepareIdxForAsmInterpreterForBaselineWithBaselineCode();
    int PrepareIdxForAsmInterpreterForBaselineWithoutBaselineCode();
    int PrepareIdxForAsmInterpreterWithBaselineCode();
    int PrepareIdxForAsmInterpreterWithoutBaselineCode();
    std::vector<GateRef> PrepareArgsForAsmInterpreter();
    std::vector<GateRef> PrepareBasicArgsForAsmInterpreter();
    std::vector<GateRef> PrepareAppendArgsForAsmInterpreter();

    void CallFastBuiltin(Label* notFastBuiltins, Label *exit, GateRef hir = Circuit::NullGate());
    std::vector<GateRef> PrepareArgsForFastBuiltin();
    std::vector<GateRef> PrepareBasicArgsForFastBuiltin();
    std::vector<GateRef> PrepareAppendArgsForFastBuiltin();
    bool IsCallModeSupportPGO() const;
    bool IsCallModeSupportCallBuiltin() const;
    bool IsSlowAotCall() const;
    bool IsFastAotCall() const;
    bool IsSlowAotCallWithBridge() const;
    bool IsFastAotCallWithBridge() const;
    bool CheckResultValueChangedWithReturn(GateRef prevResRef) const;
    void HandleProfileCall();
    void HandleProfileNativeCall();
    bool IsCallModeGetterSetter();
};

}   // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_CALL_STUB_BUILDER_H
