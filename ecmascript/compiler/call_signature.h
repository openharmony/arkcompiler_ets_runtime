/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_CALL_SIGNATURE_H
#define ECMASCRIPT_COMPILER_CALL_SIGNATURE_H

#include <array>
#include <functional>
#include <memory>

#include "ecmascript/compiler/variable_type.h"

#include "libpandabase/macros.h"
#include "libpandabase/utils/bit_field.h"

namespace panda::ecmascript::kungfu {
class Circuit;

enum class ArgumentsOrder {
    DEFAULT_ORDER,  // Push Arguments in stack from right -> left
};

class CallSignature {
public:
    using TargetConstructor = std::function<void *(void *)>;
    enum class TargetKind : uint8_t {
        COMMON_STUB = 0,
        RUNTIME_STUB,
        RUNTIME_STUB_VARARGS,
        RUNTIME_STUB_NO_GC,
        ASM_CALL_BARRIER_STUB,
        OPTIMIZED_STUB,
        OPTIMIZED_FAST_CALL_STUB,
        DEOPT_STUB,
        BYTECODE_HANDLER,
        BYTECODE_DEBUGGER_HANDLER,
        BYTECODE_HELPER_HANDLER,
        BYTECODE_PROFILE_HANDLER,
        BYTECODE_JIT_PROFILE_HANDLER,
        JSFUNCTION,
        BUILTINS_STUB,
        BUILTINS_WITH_ARGV_STUB,
        BASELINE_STUB,

        STUB_BEGIN = COMMON_STUB,
        STUB_END = BYTECODE_HANDLER,
        BCHANDLER_BEGIN = BYTECODE_HANDLER,
        BCHANDLER_END = JSFUNCTION
    };
    enum class CallConv: uint8_t {
        CCallConv = 0,
        GHCCallConv = 1,
        WebKitJSCallConv = 2,
    };
    // ParamAttr only works for LLVM backend.
    enum class ParamAttr: uint8_t {
        NoAttr = 0,
        Dead
    };
    static constexpr size_t TARGET_KIND_BIT_LENGTH = 5;
    static constexpr size_t CALL_CONV_BIT_LENGTH = 2;
    using TargetKindBit = panda::BitField<TargetKind, 0, TARGET_KIND_BIT_LENGTH>;
    using CallConvBit = TargetKindBit::NextField<CallConv, CALL_CONV_BIT_LENGTH>;
    using VariadicArgsBit = CallConvBit::NextField<bool, 1>;
    using TailCallBit = VariadicArgsBit::NextField<bool, 1>;
    using GCLeafFunctionBit = TailCallBit::NextField<bool, 1>;

    CallSignature(std::string name, int flags, size_t paramCounter, ArgumentsOrder order,
                  VariableType returnType)
        : name_(name), paramCounter_(paramCounter), order_(order), returnType_(returnType)
    {
        SetTargetKind(TargetKind::COMMON_STUB);
        SetCallConv(CallSignature::CallConv::CCallConv);
        SetTailCall(false);
        SetGCLeafFunction(false);
        SetVariadicArgs(flags);
    }

    CallSignature(std::string name, int flags, ArgumentsOrder order, VariableType returnType,
                  std::initializer_list<VariableType> params)
        : CallSignature(std::move(name), flags, params.size(), order, returnType)
    {
        paramsType_ = std::make_unique<std::vector<VariableType>>(params);
    }

    CallSignature() = default;

    ~CallSignature() = default;

    CallSignature(CallSignature const &other)
    {
        name_ = other.name_;
        paramCounter_ = other.paramCounter_;
        order_ = other.order_;
        id_ = other.id_;
        returnType_ = other.returnType_;
        constructor_ = other.constructor_;
        if (paramCounter_ > 0 && other.paramsType_ != nullptr) {
            paramsType_ = std::make_unique<std::vector<VariableType>>(paramCounter_);
            for (size_t i = 0; i < paramCounter_; i++) {
                (*paramsType_)[i] = other.GetParametersType()[i];
            }
        }
        kind_ = other.kind_;
    }

    CallSignature &operator=(CallSignature const &other)
    {
        name_ = other.name_;
        paramCounter_ = other.paramCounter_;
        order_ = other.order_;
        id_ = other.id_;
        returnType_ = other.returnType_;
        constructor_ = other.constructor_;
        if (paramCounter_ > 0 && other.paramsType_ != nullptr) {
            paramsType_ = std::make_unique<std::vector<VariableType>>(paramCounter_);
            for (size_t i = 0; i < paramCounter_; i++) {
                (*paramsType_)[i] = other.GetParametersType()[i];
            }
        }
        kind_ = other.kind_;
        return *this;
    }

    bool IsCommonStub() const
    {
        return (GetTargetKind() == TargetKind::COMMON_STUB);
    }

    bool IsRuntimeVAStub() const
    {
        return (GetTargetKind() == TargetKind::RUNTIME_STUB_VARARGS);
    }

    bool IsRuntimeStub() const
    {
        return (GetTargetKind() == TargetKind::RUNTIME_STUB);
    }

    bool IsASMCallBarrierStub() const
    {
        return (GetTargetKind() == TargetKind::ASM_CALL_BARRIER_STUB);
    }

    bool IsRuntimeNGCStub() const
    {
        return (GetTargetKind() == TargetKind::RUNTIME_STUB_NO_GC);
    }

    bool IsOptimizedStub() const
    {
        return (GetTargetKind() == TargetKind::OPTIMIZED_STUB);
    }

    bool IsOptimizedFastCallStub() const
    {
        return (GetTargetKind() == TargetKind::OPTIMIZED_FAST_CALL_STUB);
    }

    bool IsBCDebuggerStub() const
    {
        return (GetTargetKind() == TargetKind::BYTECODE_DEBUGGER_HANDLER);
    }

    bool IsStub() const
    {
        TargetKind targetKind = GetTargetKind();
        return TargetKind::STUB_BEGIN <= targetKind && targetKind < TargetKind::STUB_END;
    }

    bool IsBCStub() const
    {
        TargetKind targetKind = GetTargetKind();
        return TargetKind::BCHANDLER_BEGIN <= targetKind && targetKind < TargetKind::BCHANDLER_END;
    }

    bool IsBuiltinsStub() const
    {
        return (GetTargetKind() == TargetKind::BUILTINS_STUB);
    }

    bool IsBuiltinsWithArgvStub() const
    {
        return (GetTargetKind() == TargetKind::BUILTINS_WITH_ARGV_STUB);
    }

    bool IsBCHandlerStub() const
    {
        return (GetTargetKind() == TargetKind::BYTECODE_HANDLER);
    }

    bool IsDeoptStub() const
    {
        return (GetTargetKind() == TargetKind::DEOPT_STUB);
    }

    bool IsBaselineStub() const
    {
        return (GetTargetKind() == TargetKind::BASELINE_STUB);
    }

    void SetParameters(VariableType *paramsType)
    {
        if (paramCounter_ > 0 && paramsType_ == nullptr) {
            paramsType_ = std::make_unique<std::vector<VariableType>>(paramCounter_);
            for (size_t i = 0; i < paramCounter_; i++) {
                (*paramsType_)[i] = paramsType[i];
            }
        }
    }

    VariableType *GetParametersType() const
    {
        if (paramsType_ != nullptr) {
            return paramsType_->data();
        } else {
            return nullptr;
        }
    }

    void SetParamAttr(std::vector<ParamAttr> &&paramAttr)
    {
        if (paramCounter_ > 0 && paramsAttr_ == nullptr) {
            paramsAttr_ = std::make_unique<std::vector<ParamAttr>>(paramAttr);
        }
    }

    std::vector<ParamAttr> *GetParamAttr() const
    {
        if (paramsAttr_ != nullptr) {
            return paramsAttr_.get();
        } else {
            return nullptr;
        }
    }

    size_t GetParametersCount() const
    {
        return paramCounter_;
    }

    VariableType GetReturnType() const
    {
        return returnType_;
    }

    ArgumentsOrder GetArgumentsOrder() const
    {
        return order_;
    }

    bool IsVariadicArgs() const
    {
        return VariadicArgsBit::Decode(kind_);
    }

    void SetVariadicArgs(bool variable)
    {
        VariadicArgsBit::Set<uint64_t>(variable, &kind_);
    }

    void SetTailCall(bool tailCall)
    {
        TailCallBit::Set<uint64_t>(tailCall, &kind_);
    }

    bool GetTailCall() const
    {
        return TailCallBit::Decode(kind_);
    }

    void SetGCLeafFunction(bool value)
    {
        GCLeafFunctionBit::Set<uint64_t>(value, &kind_);
    }

    bool GetGCLeafFunction() const
    {
        return GCLeafFunctionBit::Decode(kind_);
    }

    TargetKind GetTargetKind() const
    {
        return TargetKindBit::Decode(kind_);
    }

    void SetTargetKind(TargetKind kind)
    {
        TargetKindBit::Set<uint64_t>(kind, &kind_);
    }

    CallConv GetCallConv() const
    {
        return CallConvBit::Decode(kind_);
    }

    void SetCallConv(CallConv cc)
    {
        CallConvBit::Set<uint64_t>(cc, &kind_);
    }

    const std::string &GetName() const
    {
        return name_;
    }

    void SetName(const std::string &str)
    {
        name_ = str;
    }

    void SetConstructor(TargetConstructor ctor)
    {
        constructor_ = ctor;
    }

    TargetConstructor GetConstructor() const
    {
        return constructor_;
    }

    bool HasConstructor() const
    {
        return constructor_ != nullptr;
    }

    int GetID() const
    {
        return id_;
    }

    void SetID(int id)
    {
        id_ = id;
    }

private:
    std::string name_;
    size_t paramCounter_ {0};
    int id_ {-1};
    ArgumentsOrder order_ {ArgumentsOrder::DEFAULT_ORDER};
    VariableType returnType_ {VariableType::VOID()};
    std::unique_ptr<std::vector<VariableType>> paramsType_ {nullptr};
    std::unique_ptr<std::vector<ParamAttr>> paramsAttr_ {nullptr};
    TargetConstructor constructor_ {nullptr};
    uint64_t kind_ {0};
};

#define EXPLICIT_CALL_SIGNATURE_LIST(V)         \
    V(Add)                                      \
    V(Sub)                                      \
    V(Mul)                                      \
    V(MulGCTest)                                \
    V(Div)                                      \
    V(Mod)                                      \
    V(TypeOf)                                   \
    V(Equal)                                    \
    V(NotEqual)                                 \
    V(StrictEqual)                              \
    V(StrictNotEqual)                           \
    V(Less)                                     \
    V(LessEq)                                   \
    V(Greater)                                  \
    V(GreaterEq)                                \
    V(Shl)                                      \
    V(Shr)                                      \
    V(Ashr)                                     \
    V(And)                                      \
    V(Or)                                       \
    V(Xor)                                      \
    V(IsIn)                                     \
    V(Instanceof)                               \
    V(Inc)                                      \
    V(Dec)                                      \
    V(Neg)                                      \
    V(Not)                                      \
    V(ToBooleanTrue)                            \
    V(ToBooleanFalse)                           \
    V(SetPropertyByName)                        \
    V(DeprecatedSetPropertyByName)              \
    V(SetPropertyByNameWithOwn)                 \
    V(SetPropertyByValue)                       \
    V(DeprecatedSetPropertyByValue)             \
    V(TryLdGlobalByName)                        \
    V(TryStGlobalByName)                        \
    V(LdGlobalVar)                              \
    V(StGlobalVar)                              \
    V(StOwnByIndex)                             \
    V(StOwnByValue)                             \
    V(StOwnByName)                              \
    V(StOwnByValueWithNameSet)                  \
    V(StOwnByNameWithNameSet)                   \
    V(StObjByIndex)                             \
    V(LdObjByIndex)                             \
    V(JSTaggedValueHasProperty)                 \
    V(SetPropertyByValueWithOwn)                \
    V(GetPropertyByName)                        \
    V(SetPropertyByNameWithMega)                \
    V(GetPropertyByNameWithMega)                \
    V(DeprecatedGetPropertyByName)              \
    V(GetPropertyByIndex)                       \
    V(SetPropertyByIndex)                       \
    V(SetPropertyByIndexWithOwn)                \
    V(GetPropertyByValue)                       \
    V(DeprecatedGetPropertyByValue)             \
    V(TryLoadICByName)                          \
    V(TryLoadICByValue)                         \
    V(TryStoreICByName)                         \
    V(TryStoreICByValue)                        \
    V(SetValueWithBarrier)                      \
    V(SetNonSValueWithBarrier)                  \
    V(SetSValueWithBarrier)                     \
    V(NewLexicalEnv)                            \
    V(CopyRestArgs)                             \
    V(CreateObjectHavingMethod)                 \
    V(GetUnmappedArgs)                          \
    V(GetCallSpreadArgs)                        \
    V(NewThisObjectChecked)                     \
    V(ConstructorCheck)                         \
    V(CreateEmptyArray)                         \
    V(CreateArrayWithBuffer)                    \
    V(NewJSObject)                              \
    V(GetTaggedArrayPtrTest)                    \
    V(BytecodeHandler)                          \
    V(Builtins)                                 \
    V(BuiltinsWithArgv)                         \
    V(BytecodeDebuggerHandler)                  \
    V(CallRuntime)                              \
    V(AsmInterpreterEntry)                      \
    V(CallArg0AndCheckToBaseline)               \
    V(CallArg1AndCheckToBaseline)               \
    V(CallArgs2AndCheckToBaseline)              \
    V(CallArgs3AndCheckToBaseline)              \
    V(CallThisArg0AndCheckToBaseline)           \
    V(CallThisArg1AndCheckToBaseline)           \
    V(CallThisArgs2AndCheckToBaseline)          \
    V(CallThisArgs3AndCheckToBaseline)          \
    V(CallRangeAndCheckToBaseline)              \
    V(CallNewAndCheckToBaseline)                \
    V(SuperCallAndCheckToBaseline)              \
    V(CallThisRangeAndCheckToBaseline)          \
    V(GeneratorReEnterAsmInterp)                \
    V(CallRuntimeWithArgv)                      \
    V(OptimizedCallAndPushArgv)                 \
    V(OptimizedFastCallAndPushArgv)             \
    V(AOTCallToAsmInterBridge)                  \
    V(FastCallToAsmInterBridge)                 \
    V(PushCallArg0AndDispatch)                  \
    V(PushCallArgsAndDispatchNative)            \
    V(PushCallArg1AndDispatch)                  \
    V(PushCallArgs2AndDispatch)                 \
    V(PushCallArgs3AndDispatch)                 \
    V(PushCallRangeAndDispatch)                 \
    V(PushCallRangeAndDispatchNative)           \
    V(PushCallThisRangeAndDispatch)             \
    V(PushCallThisArg0AndDispatch)              \
    V(PushCallThisArg1AndDispatch)              \
    V(PushCallThisArgs2AndDispatch)             \
    V(PushCallThisArgs3AndDispatch)             \
    V(PushCallNewAndDispatchNative)             \
    V(PushNewTargetAndDispatchNative)           \
    V(PushCallNewAndDispatch)                   \
    V(PushSuperCallAndDispatch)                 \
    V(CallGetter)                               \
    V(CallSetter)                               \
    V(CallContainersArgs2)                      \
    V(CallContainersArgs3)                      \
    V(CallGetterToBaseline)                     \
    V(CallSetterToBaseline)                     \
    V(CallContainersArgs2ToBaseline)            \
    V(CallContainersArgs3ToBaseline)            \
    V(CallReturnWithArgvToBaseline)             \
    V(JSCallWithArgV)                           \
    V(JSFastCallWithArgV)                       \
    V(JSFastCallWithArgVAndPushArgv)            \
    V(JSCallWithArgVAndPushArgv)                \
    V(SuperCallWithArgV)                        \
    V(ResumeRspAndDispatch)                     \
    V(ResumeRspAndReturn)                       \
    V(ResumeRspAndReturnBaseline)               \
    V(ResumeCaughtFrameAndDispatch)             \
    V(ResumeUncaughtFrameAndReturn)             \
    V(ResumeRspAndRollback)                     \
    V(BigIntEquals)                             \
    V(BigIntSameValueZero)                      \
    V(CallBigIntAsIntN)                         \
    V(CallBigIntAsUintN)                        \
    V(Dump)                                     \
    V(DebugDump)                                \
    V(DumpWithHint)                             \
    V(DebugDumpWithHint)                        \
    V(DebugPrint)                               \
    V(DebugPrintCustom)                         \
    V(DebugPrintInstruction)                    \
    V(CollectingOpcodes)                        \
    V(DebugOsrEntry)                            \
    V(Comment)                                  \
    V(FatalPrint)                               \
    V(FatalPrintCustom)                         \
    V(GetActualArgvNoGC)                        \
    V(InsertOldToNewRSet)                       \
    V(InsertLocalToShareRSet)                   \
    V(SetBitAtomic)                             \
    V(DoubleToInt)                              \
    V(SaturateTruncDoubleToInt32)            \
    V(FloatMod)                                 \
    V(FloatAcos)                                \
    V(FloatAcosh)                               \
    V(FloatAsin)                                \
    V(FloatAsinh)                               \
    V(FloatAtan)                                \
    V(FloatAtan2)                               \
    V(FloatAtanh)                               \
    V(FloatCos)                                 \
    V(FloatCosh)                                \
    V(FloatSin)                                 \
    V(FloatSinh)                                \
    V(FloatTan)                                 \
    V(FloatTanh)                                \
    V(FloatTrunc)                               \
    V(FloatLog)                                 \
    V(FloatLog2)                                \
    V(FloatLog10)                               \
    V(FloatLog1p)                               \
    V(FloatExp)                                 \
    V(FloatExpm1)                               \
    V(FloatCbrt)                                \
    V(FloatClz32)                               \
    V(FloatFloor)                               \
    V(FloatPow)                                 \
    V(FloatCeil)                                \
    V(CallDateNow)                              \
    V(UpdateFieldType)                          \
    V(MarkingBarrier)                           \
    V(SharedGCMarkingBarrier)                   \
    V(CallArg0)                                 \
    V(CallArg1)                                 \
    V(CallArgs2)                                \
    V(CallArgs3)                                \
    V(CallThisRange)                            \
    V(CallRange)                                \
    V(JSCall)                                   \
    V(JSOptimizedCall)                          \
    V(JSOptimizedFastCall)                      \
    V(JSFunctionEntry)                          \
    V(OptimizedFastCallEntry)                   \
    V(JSProxyCallInternalWithArgV)              \
    V(JSObjectGetMethod)                        \
    V(JsProxyCallInternal)                      \
    V(JsBoundCallInternal)                      \
    V(DeoptHandlerAsm)                          \
    V(JSCallNew)                                \
    V(CallOptimized)                            \
    V(TimeClip)                                 \
    V(SetDateValues)                            \
    V(CallReturnWithArgv)                       \
    V(StartCallTimer)                           \
    V(EndCallTimer)                             \
    V(GetSingleCharCodeByIndex)                 \
    V(CreateStringBySingleCharCode)             \
    V(FastStringEqual)                          \
    V(FastStringAdd)                            \
    V(StringAdd)                                \
    V(DeleteObjectProperty)                     \
    V(Getpropiterator)                          \
    V(Getnextpropname)                          \
    V(CreateJSSetIterator)                      \
    V(CreateJSMapIterator)                      \
    V(JSMapGet)                                 \
    V(JSMapHas)                                 \
    V(JSMapKeys)                                \
    V(JSMapValues)                              \
    V(JSSetHas)                                 \
    V(JSSetAdd)                                 \
    V(JSProxyGetProperty)                       \
    V(JSProxySetProperty)                       \
    V(GrowElementsCapacity)                     \
    V(CreateJSTypedArrayEntries)                \
    V(CreateJSTypedArrayKeys)                   \
    V(CreateJSTypedArrayValues)                 \
    V(JSMapDelete)                              \
    V(JSSetDelete)                              \
    V(JSSetEntries)                             \
    V(JSHClassFindProtoTransitions)             \
    V(FinishObjSizeTracking)                    \
    V(NumberHelperStringToDouble)               \
    V(GetStringToListCacheArray)                \
    V(IntLexicographicCompare)                  \
    V(DoubleLexicographicCompare)               \
    V(FastArraySortString)                      \
    V(StringToNumber)                           \
    V(StringGetStart)                           \
    V(StringGetEnd)                             \
    V(ArrayTrim)                                \
    V(CopyTypedArrayBuffer)                     \
    V(OptimizedFastJmp)                         \
    V(CallArg0AndDispatchFromBaseline)          \
    V(CallArg1AndDispatchFromBaseline)          \
    V(CallArgs2AndDispatchFromBaseline)         \
    V(CallArgs3AndDispatchFromBaseline)         \
    V(CallThisArg0AndDispatchFromBaseline)      \
    V(CallThisArg1AndDispatchFromBaseline)      \
    V(CallThisArgs2AndDispatchFromBaseline)     \
    V(CallThisArgs3AndDispatchFromBaseline)     \
    V(CallRangeAndDispatchFromBaseline)         \
    V(CallNewAndDispatchFromBaseline)           \
    V(SuperCallAndDispatchFromBaseline)         \
    V(CallThisRangeAndDispatchFromBaseline)     \
    V(CallArg0AndCheckToBaselineFromBaseline)        \
    V(CallArg1AndCheckToBaselineFromBaseline)        \
    V(CallArgs2AndCheckToBaselineFromBaseline)       \
    V(CallArgs3AndCheckToBaselineFromBaseline)       \
    V(CallThisArg0AndCheckToBaselineFromBaseline)    \
    V(CallThisArg1AndCheckToBaselineFromBaseline)    \
    V(CallThisArgs2AndCheckToBaselineFromBaseline)   \
    V(CallThisArgs3AndCheckToBaselineFromBaseline)   \
    V(CallRangeAndCheckToBaselineFromBaseline)       \
    V(CallNewAndCheckToBaselineFromBaseline)         \
    V(SuperCallAndCheckToBaselineFromBaseline)       \
    V(CallThisRangeAndCheckToBaselineFromBaseline)   \
    V(SameValue)                                     \
    V(GetBaselineBuiltinFp)                          \
    V(StringIteratorNext)                            \
    V(Definefunc)                                    \
    V(DefineField)                                   \
    V(CallArg0Stub)                                  \
    V(CallArg1Stub)                                  \
    V(CallArg2Stub)                                  \
    V(CallArg3Stub)                                  \
    V(CallThis0Stub)                                 \
    V(CallThis1Stub)                                 \
    V(CallThis2Stub)                                 \
    V(CallThis3Stub)                                 \
    V(ConvertCharToInt32)                            \
    V(ConvertCharToDouble)                           \
    V(ASMFastWriteBarrier)                           \
    V(VerifyBarrier)                                 \
    V(SortTypedArray)                                \
    V(ReverseTypedArray)                             \
    V(IsFastRegExp)                                  \
    V(MapIteratorNext)                               \
    V(SetIteratorNext)                               \
    V(CreateLocalToShare)                            \
    V(CreateOldToNew)                                \
    V(BatchBarrier)                                  \
    V(ObjectCopy)                                    \
    V(ArrayIteratorNext)                             \
    V(GetIterator)                                   \
    V(MoveBarrierInRegion)                           \
    V(FillObject)                                    \
    V(MoveBarrierCrossRegion)                        \
    V(ReverseArray)                                  \
    V(LrInt)                                         \
    V(FindEntryFromNameDictionary)                   \
    V(ReverseBarrier)                                \
    V(FindPatchModule)                               \
    V(FatalPrintMisstakenResolvedBinding)            \
    V(LoadNativeModuleFailed)

#define DECL_CALL_SIGNATURE(name)                                  \
class name##CallSignature final {                                  \
    public:                                                        \
        static void Initialize(CallSignature *descriptor);         \
    };
EXPLICIT_CALL_SIGNATURE_LIST(DECL_CALL_SIGNATURE)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEF_CALL_SIGNATURE(name)                                  \
    void name##CallSignature::Initialize([[maybe_unused]] CallSignature *callSign)
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_CALL_SIGNATURE_H
