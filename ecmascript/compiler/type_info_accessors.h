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

#ifndef ECMASCRIPT_COMPILER_TYPE_INFO_ACCESSORS_H
#define ECMASCRIPT_COMPILER_TYPE_INFO_ACCESSORS_H

#include "ecmascript/compiler/aot_compiler_preprocessor.h"
#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/pgo_type/pgo_type_manager.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/enum_conversion.h"
#include "ecmascript/global_index.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/mem/chunk.h"
#include "libpandafile/index_accessor.h"

namespace panda::ecmascript::kungfu {
class TypeInfoAccessor {
public:
    TypeInfoAccessor(const CompilationEnv *env, Circuit* circuit, GateRef gate)
        : compilationEnv_(env),
          acc_(circuit),
          argAcc_(circuit),
          gate_(gate)
    {
        pgoType_ = acc_.TryGetPGOType(gate);
        // NOTICE-PGO: wx delete in part3
        ptManager_ = compilationEnv_->GetPTManager();
        isAot_ = compilationEnv_->IsAotCompiler();
    }

    inline GateRef GetGate() const
    {
        return gate_;
    }

    uint32_t TryGetHeapConstantFunctionIndex(uint32_t callMethodId) const
    {
        if (compilationEnv_ == nullptr || !compilationEnv_->SupportHeapConstant()) {
            return JitCompilationEnv::INVALID_HEAP_CONSTANT_INDEX;
        }
        ASSERT(compilationEnv_->IsJitCompiler());
        auto *jitCompilationEnv = static_cast<const JitCompilationEnv*>(compilationEnv_);
        const auto &callMethodId2HeapConstantIndex = jitCompilationEnv->GetCallMethodId2HeapConstantIndex();
        auto itr = callMethodId2HeapConstantIndex.find(callMethodId);
        if (itr != callMethodId2HeapConstantIndex.end()) {
            return itr->second;
        }
        return JitCompilationEnv::INVALID_HEAP_CONSTANT_INDEX;
    }

    uint32_t TryGetInlineHeapConstantFunctionIndex(uint32_t callMethodId) const
    {
        if (compilationEnv_ == nullptr || !compilationEnv_->SupportHeapConstant()) {
            return JitCompilationEnv::INVALID_HEAP_CONSTANT_INDEX;
        }
        ASSERT(compilationEnv_->IsJitCompiler());
        auto *jitCompilationEnv = static_cast<const JitCompilationEnv*>(compilationEnv_);
        const auto &callMethodId2HeapConstantIndex = jitCompilationEnv->GetCallMethodId2HeapConstantIndex();
        auto itr = callMethodId2HeapConstantIndex.find(callMethodId);
        if (itr != callMethodId2HeapConstantIndex.end()) {
            return itr->second;
        }
        const auto &onlyInlineMethodId2HeapConstantIndex = jitCompilationEnv->GetOnlyInlineMethodId2HeapConstantIndex();
        auto iter = onlyInlineMethodId2HeapConstantIndex.find(callMethodId);
        if (iter != onlyInlineMethodId2HeapConstantIndex.end()) {
            return iter->second;
        }
        return JitCompilationEnv::INVALID_HEAP_CONSTANT_INDEX;
    }

    static bool IsTrustedBooleanType(GateAccessor acc, GateRef gate);

    static bool IsTrustedNumberType(GateAccessor acc, GateRef gate);

    static bool IsTrustedStringType(
        const CompilationEnv *env, Circuit *circuit, Chunk *chunk, GateAccessor acc, GateRef gate);

    static inline bool IsTrustedBooleanOrNumberOrStringType(const CompilationEnv *env, Circuit *circuit,
                                                            Chunk *chunk, GateAccessor acc, GateRef gate)
    {
        return IsTrustedBooleanType(acc, gate) || IsTrustedNumberType(acc, gate) ||
               IsTrustedStringType(env, circuit, chunk, acc, gate);
    }

    inline bool IsAot() const
    {
        return isAot_;
    }

    static bool IsTrustedNotSameType(const CompilationEnv *env, Circuit *circuit, Chunk *chunk,
                                     GateAccessor acc, GateRef left, GateRef right);

    BuiltinsStubCSigns::ID TryGetPGOBuiltinMethodId() const;

    static constexpr uint32_t INVALID_LEN = std::numeric_limits<uint32_t>::max();

protected:
    ParamType PGOSampleTypeToParamType() const;
    static ParamType PGOBuiltinTypeToParamType(ProfileType pgoType);
    bool IsMegaType() const;

    const CompilationEnv *compilationEnv_ {nullptr};
    GateAccessor acc_;
    ArgumentAccessor argAcc_;
    GateRef gate_;
    PGOTypeRef pgoType_;
    // NOTICE-PGO: wx delete in part3
    PGOTypeManager *ptManager_ {nullptr};
    bool isAot_;
};

class BinOpTypeInfoAccessor final : public TypeInfoAccessor {
public:
    BinOpTypeInfoAccessor(const CompilationEnv *env,
                          Circuit *circuit,
                          GateRef gate)
        : TypeInfoAccessor(env, circuit, gate)
    {
        left_ = acc_.GetValueIn(gate, 0);   // 0: left
        right_ = acc_.GetValueIn(gate, 1);  // 1: right
    }
    NO_COPY_SEMANTIC(BinOpTypeInfoAccessor);
    NO_MOVE_SEMANTIC(BinOpTypeInfoAccessor);

    inline GateRef GetLeftGate() const
    {
        return left_;
    }

    inline GateRef GetReightGate() const
    {
        return right_;
    }

    inline bool HasNumberType() const
    {
        if (LeftOrRightIsUndefinedOrNull()) {
            return false;
        }
        return pgoType_.HasNumber();
    }

    inline bool IsStringType() const
    {
        if (LeftOrRightIsUndefinedOrNull()) {
            return false;
        }
        return pgoType_.IsString();
    }

    inline bool IsInternStringType() const
    {
        if (LeftOrRightIsUndefinedOrNull()) {
            return false;
        }
        return pgoType_.IsInternString();
    }

    inline bool IsNumberOrStringType() const
    {
        if (LeftOrRightIsUndefinedOrNull()) {
            return false;
        }
        return pgoType_.IsNumberOrString();
    }

    inline bool LeftOrRightIsUndefinedOrNull() const
    {
        return acc_.IsUndefinedOrNullOrHole(left_) || acc_.IsUndefinedOrNullOrHole(right_);
    }

    inline ParamType GetParamType() const
    {
        if (LeftOrRightIsUndefinedOrNull()) {
            return ParamType::AnyType();
        }
        return PGOSampleTypeToParamType();
    }

private:
    GateRef left_;
    GateRef right_;
};

class UnOpTypeInfoAccessor : public TypeInfoAccessor {
public:
    UnOpTypeInfoAccessor(const CompilationEnv *env,
                         Circuit *circuit,
                         GateRef gate)
        : TypeInfoAccessor(env, circuit, gate)
    {
        value_ = acc_.GetValueIn(gate, 0); // 0: value
    }
    NO_COPY_SEMANTIC(UnOpTypeInfoAccessor);
    NO_MOVE_SEMANTIC(UnOpTypeInfoAccessor);

    inline GateRef GetValue() const
    {
        return value_;
    }

    inline bool HasNumberType() const
    {
        return pgoType_.HasNumber();
    }

    inline bool IsBooleanType() const
    {
        return pgoType_.IsBoolean();
    }

    inline bool IsUndefinedOrNullType() const
    {
        return pgoType_.IsUndefinedOrNull();
    }

    inline ParamType GetParamType() const
    {
        return PGOSampleTypeToParamType();
    }

    GateType GetValueGateType() const
    {
        return acc_.GetGateType(value_);
    }

protected:
    GateRef value_ {Circuit::NullGate()};
};

class ConditionJumpTypeInfoAccessor final : public UnOpTypeInfoAccessor {
public:
    ConditionJumpTypeInfoAccessor(const CompilationEnv *env,
                                  Circuit *circuit,
                                  GateRef gate)
        : UnOpTypeInfoAccessor(env, circuit, gate) {}
    NO_COPY_SEMANTIC(ConditionJumpTypeInfoAccessor);
    NO_MOVE_SEMANTIC(ConditionJumpTypeInfoAccessor);

    uint32_t GetBranchWeight() const
    {
        return acc_.TryGetPGOType(value_).GetPGOSampleType()->GetWeight();
    }
};

class NewObjRangeTypeInfoAccessor final : public UnOpTypeInfoAccessor {
public:
    class AccessorStrategy {
    public:
        virtual ~AccessorStrategy() = default;
        virtual bool IsValidCallMethodId() const = 0;
        virtual uint32_t GetCallMethodId() const = 0;
        virtual bool FindHClass() const = 0;
        virtual JSTaggedValue GetHClass() const = 0;
    };

    class AotAccessorStrategy : public AccessorStrategy {
    public:
        explicit AotAccessorStrategy(NewObjRangeTypeInfoAccessor &parent) : parent_(parent)
        {
        }

        bool IsValidCallMethodId() const override
        {
            return parent_.pgoType_.IsValidCallMethodId();
        }

        uint32_t GetCallMethodId() const override
        {
            ASSERT(IsValidCallMethodId());
            return parent_.pgoType_.GetCallMethodId();
        }
        bool FindHClass() const override;
        JSTaggedValue GetHClass() const override;

    private:
        NewObjRangeTypeInfoAccessor &parent_;
    };

    class JitAccessorStrategy : public AccessorStrategy {
    public:
        explicit JitAccessorStrategy(NewObjRangeTypeInfoAccessor &parent) : parent_(parent)
        {
        }

        bool IsValidCallMethodId() const override
        {
            return parent_.pgoType_.IsDefOpValidCallMethodId();
        }

        uint32_t GetCallMethodId() const override
        {
            ASSERT(IsValidCallMethodId());
            return parent_.pgoType_.GetDefOpCallMethodId();
        }
        bool FindHClass() const override;
        JSTaggedValue GetHClass() const override;

    private:
        NewObjRangeTypeInfoAccessor &parent_;
    };

    NewObjRangeTypeInfoAccessor(const CompilationEnv *env, Circuit *circuit, GateRef gate, Chunk* chunk)
        : UnOpTypeInfoAccessor(env, circuit, gate), hclassIndex_(-1), traHClassIndex_(-1)
    {
        if (IsAot()) {
            strategy_ = chunk->New<AotAccessorStrategy>(*this);
        } else {
            strategy_ = chunk->New<JitAccessorStrategy>(*this);
        }
    }
    NO_COPY_SEMANTIC(NewObjRangeTypeInfoAccessor);
    NO_MOVE_SEMANTIC(NewObjRangeTypeInfoAccessor);

    bool FindHClass()
    {
        return strategy_->FindHClass();
    }
    JSTaggedValue GetHClass()
    {
        return strategy_->GetHClass();
    }

    bool IsValidCallMethodId() const
    {
        return strategy_->IsValidCallMethodId();
    }

    uint32_t GetCallMethodId() const
    {
        ASSERT(IsValidCallMethodId());
        return strategy_->GetCallMethodId();
    }

    int GetHClassIndex() const
    {
        if (traHClassIndex_ != -1) {
            return traHClassIndex_;
        }
        return hclassIndex_;
    }

private:
    int hclassIndex_;
    int traHClassIndex_ {-1};
    AccessorStrategy* strategy_;

    friend class AotAccessorStrategy;
    friend class JitAccessorStrategy;
};

class NewBuiltinCtorTypeInfoAccessor final : public UnOpTypeInfoAccessor {
public:
    NewBuiltinCtorTypeInfoAccessor(const CompilationEnv *env,
                                   Circuit *circuit,
                                   GateRef gate)
        : UnOpTypeInfoAccessor(env, circuit, gate) {}
    NO_COPY_SEMANTIC(NewBuiltinCtorTypeInfoAccessor);
    NO_MOVE_SEMANTIC(NewBuiltinCtorTypeInfoAccessor);

    bool IsBuiltinId(BuiltinsStubCSigns::ID id)
    {
        if (pgoType_.IsPGOSampleType()) {
            return TryGetPGOBuiltinMethodId() == id;
        } else {
            return false;
        }
    }

private:
    GlobalTSTypeRef GetCtorGT() const
    {
        return GetValueGateType().GetGTRef();
    }
};

class TypeOfTypeInfoAccessor final : public UnOpTypeInfoAccessor {
public:
    TypeOfTypeInfoAccessor(const CompilationEnv *env,
                           Circuit *circuit,
                           GateRef gate)
        : UnOpTypeInfoAccessor(env, circuit, gate) {}
    NO_COPY_SEMANTIC(TypeOfTypeInfoAccessor);
    NO_MOVE_SEMANTIC(TypeOfTypeInfoAccessor);

    bool IsIllegalType() const;
};

class SuperCallTypeInfoAccessor final : public TypeInfoAccessor {
public:
    SuperCallTypeInfoAccessor(const CompilationEnv *env,
                              Circuit *circuit,
                              GateRef gate,
                              const JSPandaFile *jsPandaFile = nullptr,
                              const CallMethodFlagMap *callMethodFlagMap = nullptr);
    NO_COPY_SEMANTIC(SuperCallTypeInfoAccessor);
    NO_MOVE_SEMANTIC(SuperCallTypeInfoAccessor);

    bool IsValidCallMethodId() const
    {
        return pgoType_.IsValidCallMethodId();
    }

    uint32_t GetMethodId() const
    {
        if (jsPandaFile_ == nullptr || callMethodFlagMap_ == nullptr) {
            return 0;
        }
        auto profileType = acc_.TryGetPGOType(gate_).GetPGOSampleType();
        if (!profileType->IsNone()) {
            return profileType->GetProfileType().GetId();
        }
        return 0;
    }

    GateRef GetCtor() const
    {
        return ctor_;
    }

protected:
    const JSPandaFile *jsPandaFile_;
    const CallMethodFlagMap *callMethodFlagMap_;

private:
    GateRef ctor_;
};

class CallTypeInfoAccessor : public TypeInfoAccessor {
public:
    CallTypeInfoAccessor(const CompilationEnv *env,
                         Circuit *circuit,
                         GateRef gate,
                         const JSPandaFile *jsPandaFile = nullptr,
                         const CallMethodFlagMap *callMethodFlagMap = nullptr)
        : TypeInfoAccessor(env, circuit, gate),
          argc_(0),
          func_(Circuit::NullGate()),
          jsPandaFile_(jsPandaFile),
          callMethodFlagMap_(callMethodFlagMap)
    {}

    size_t GetArgc() const
    {
        return argc_;
    }

    GateRef GetFunc() const
    {
        return func_;
    }

    GateType GetFuncGateType() const
    {
        return acc_.GetGateType(func_);
    }

    bool IsValidCallMethodId() const
    {
        return pgoType_.IsValidCallMethodId();
    }

    bool IsHotnessFunc() const
    {
        if (jsPandaFile_ == nullptr || callMethodFlagMap_ == nullptr) {
            return false;
        }
        auto profileType = acc_.TryGetPGOType(gate_).GetPGOSampleType();
        bool haveProfileType = !profileType->IsNone();
        if (haveProfileType) {
            CString fileDesc = jsPandaFile_->GetNormalizedFileDesc();
            uint32_t methodId = profileType->GetProfileType().GetId();
            return callMethodFlagMap_->IsAotCompile(fileDesc, methodId);
        }
        return false;
    }

    uint32_t GetFunctionTypeLength() const
    {
        if (jsPandaFile_ == nullptr || callMethodFlagMap_ == nullptr) {
            return INVALID_LEN;
        }
        auto profileType = acc_.TryGetPGOType(gate_).GetPGOSampleType();
        bool haveProfileType = !profileType->IsNone();
        if (haveProfileType) {
            uint32_t methodId = profileType->GetProfileType().GetId();
            MethodLiteral *targetMethodLiteral = jsPandaFile_->FindMethodLiteral(methodId);
            if (UNLIKELY(targetMethodLiteral == nullptr)) {
                return INVALID_LEN;
            }

            return targetMethodLiteral->GetNumArgsWithCallField();
        }
        return INVALID_LEN;
    }

    bool IsNoGC() const
    {
        if (jsPandaFile_ == nullptr || callMethodFlagMap_ == nullptr) {
            return false;
        }
        auto profileType = acc_.TryGetPGOType(gate_).GetPGOSampleType();
        bool haveProfileType = !profileType->IsNone();
        if (haveProfileType) {
            uint32_t methodId = profileType->GetProfileType().GetId();
            MethodLiteral *targetMethodLiteral = jsPandaFile_->FindMethodLiteral(methodId);
            if (UNLIKELY(targetMethodLiteral == nullptr)) {
                return false;
            }
            return targetMethodLiteral->IsNoGC();
        }
        return false;
    }

    int GetMethodIndex() const
    {
        if (jsPandaFile_ == nullptr || callMethodFlagMap_ == nullptr) {
            return -1;
        }
        auto profileType = acc_.TryGetPGOType(gate_).GetPGOSampleType();
        bool haveProfileType = !profileType->IsNone();
        if (haveProfileType) {
            uint32_t methodId = profileType->GetProfileType().GetId();
            panda_file::IndexAccessor indexAccessor(*(jsPandaFile_->GetPandaFile()),
                                            panda_file::File::EntityId(methodId));
            uint32_t cpId = static_cast<uint32_t>(indexAccessor.GetHeaderIndex());
            ConstantPool *constpoolHandle =
                ConstantPool::Cast(compilationEnv_->FindConstpool(jsPandaFile_, cpId).GetTaggedObject());
            return constpoolHandle->GetMethodIndexByEntityId(panda_file::File::EntityId(methodId));
        }
        return -1;
    }

    const JSPandaFile *GetPandaFile() const
    {
        return jsPandaFile_;
    }

    uint32_t GetMethodId() const
    {
        if (jsPandaFile_ == nullptr || callMethodFlagMap_ == nullptr) {
            return 0;
        }
        auto profileType = acc_.TryGetPGOType(gate_).GetPGOSampleType();
        if (!profileType->IsNone()) {
            return profileType->GetProfileType().GetId();
        }
        return 0;
    }

    bool MethodOffsetIsVaild() const
    {
        auto profileType = acc_.TryGetPGOType(gate_).GetPGOSampleType();
        return !profileType->IsNone();
    }

    bool CanFastCall() const
    {
        if (jsPandaFile_ == nullptr || callMethodFlagMap_ == nullptr) {
            return false;
        }
        auto profileType = acc_.TryGetPGOType(gate_).GetPGOSampleType();
        bool haveProfileType = !profileType->IsNone();
        if (haveProfileType) {
            CString fileDesc = jsPandaFile_->GetNormalizedFileDesc();
            uint32_t methodId = profileType->GetProfileType().GetId();
            return (callMethodFlagMap_->IsAotCompile(fileDesc, methodId) ||
                    callMethodFlagMap_->IsJitCompile(fileDesc, methodId)) &&
                   callMethodFlagMap_->IsFastCall(fileDesc, methodId);
        }
        return false;
    }

    uint32_t GetFuncMethodOffset() const
    {
        if (jsPandaFile_ == nullptr || callMethodFlagMap_ == nullptr) {
            return false;
        }
        auto profileType = acc_.TryGetPGOType(gate_).GetPGOSampleType();
        bool haveProfileType = !profileType->IsNone();
        if (haveProfileType) {
            uint32_t methodId = profileType->GetProfileType().GetId();
            MethodLiteral *targetMethodLiteral = jsPandaFile_->FindMethodLiteral(methodId);
            if (UNLIKELY(targetMethodLiteral == nullptr)) {
                return false;
            }
            return targetMethodLiteral->GetMethodId().GetOffset();
        }
        return 0;
    }

protected:
    size_t argc_;
    GateRef func_;
    const JSPandaFile *jsPandaFile_;
    const CallMethodFlagMap *callMethodFlagMap_;
};

class GetIteratorTypeInfoAccessor final : public CallTypeInfoAccessor {
public:
    GetIteratorTypeInfoAccessor(const CompilationEnv *env,
                             Circuit *circuit,
                             GateRef gate,
                             const JSPandaFile *jsPandaFile = nullptr,
                             const CallMethodFlagMap *callMethodFlagMap = nullptr);
    NO_COPY_SEMANTIC(GetIteratorTypeInfoAccessor);
    NO_MOVE_SEMANTIC(GetIteratorTypeInfoAccessor);

    GateRef GetCallee()
    {
        return func_;
    }
};

class CallArg0TypeInfoAccessor final : public CallTypeInfoAccessor {
public:
    CallArg0TypeInfoAccessor(const CompilationEnv *env,
                             Circuit *circuit,
                             GateRef gate,
                             const JSPandaFile *jsPandaFile = nullptr,
                             const CallMethodFlagMap *callMethodFlagMap = nullptr);
    NO_COPY_SEMANTIC(CallArg0TypeInfoAccessor);
    NO_MOVE_SEMANTIC(CallArg0TypeInfoAccessor);
};

class CallArg1TypeInfoAccessor final : public CallTypeInfoAccessor {
public:
    CallArg1TypeInfoAccessor(const CompilationEnv *env,
                             Circuit *circuit,
                             GateRef gate,
                             const JSPandaFile *jsPandaFile = nullptr,
                             const CallMethodFlagMap *callMethodFlagMap = nullptr);
    NO_COPY_SEMANTIC(CallArg1TypeInfoAccessor);
    NO_MOVE_SEMANTIC(CallArg1TypeInfoAccessor);

    GateRef GetValue()
    {
        return value_;
    }

    GateType GetValueGateType()
    {
        return acc_.GetGateType(value_);
    }

private:
    GateRef value_;
};

class CallArg2TypeInfoAccessor final : public CallTypeInfoAccessor {
public:
    CallArg2TypeInfoAccessor(const CompilationEnv *env,
                             Circuit *circuit,
                             GateRef gate,
                             const JSPandaFile *jsPandaFile = nullptr,
                             const CallMethodFlagMap *callMethodFlagMap = nullptr);
    NO_COPY_SEMANTIC(CallArg2TypeInfoAccessor);
    NO_MOVE_SEMANTIC(CallArg2TypeInfoAccessor);
};

class CallArg3TypeInfoAccessor final : public CallTypeInfoAccessor {
public:
    CallArg3TypeInfoAccessor(const CompilationEnv *env,
                             Circuit *circuit,
                             GateRef gate,
                             const JSPandaFile *jsPandaFile = nullptr,
                             const CallMethodFlagMap *callMethodFlagMap = nullptr);
    NO_COPY_SEMANTIC(CallArg3TypeInfoAccessor);
    NO_MOVE_SEMANTIC(CallArg3TypeInfoAccessor);
};

class CallRangeTypeInfoAccessor final : public CallTypeInfoAccessor {
public:
    CallRangeTypeInfoAccessor(const CompilationEnv *env,
                              Circuit *circuit,
                              GateRef gate,
                              const JSPandaFile *jsPandaFile = nullptr,
                              const CallMethodFlagMap *callMethodFlagMap = nullptr);
    NO_COPY_SEMANTIC(CallRangeTypeInfoAccessor);
    NO_MOVE_SEMANTIC(CallRangeTypeInfoAccessor);
};

class CallThisTypeInfoAccessor : public CallTypeInfoAccessor {
public:
    CallThisTypeInfoAccessor(const CompilationEnv *env,
                             Circuit *circuit,
                             GateRef gate,
                             const JSPandaFile *jsPandaFile = nullptr,
                             const CallMethodFlagMap *callMethodFlagMap = nullptr)
        : CallTypeInfoAccessor(env, circuit, gate, jsPandaFile, callMethodFlagMap)
    {
        thisObj_ = acc_.GetValueIn(gate, 0);
    }
    NO_COPY_SEMANTIC(CallThisTypeInfoAccessor);
    NO_MOVE_SEMANTIC(CallThisTypeInfoAccessor);

    bool CanOptimizeAsFastCall();

    GateRef GetThisObj() const
    {
        return thisObj_;
    }
protected:
    GateRef thisObj_;
};

class CallThis0TypeInfoAccessor final : public CallThisTypeInfoAccessor {
public:
    CallThis0TypeInfoAccessor(const CompilationEnv *env,
                              Circuit *circuit,
                              GateRef gate,
                              const JSPandaFile *jsPandaFile = nullptr,
                              const CallMethodFlagMap *callMethodFlagMap = nullptr);
    NO_COPY_SEMANTIC(CallThis0TypeInfoAccessor);
    NO_MOVE_SEMANTIC(CallThis0TypeInfoAccessor);
};

class CallThis1TypeInfoAccessor final : public CallThisTypeInfoAccessor {
public:
    CallThis1TypeInfoAccessor(const CompilationEnv *env,
                              Circuit *circuit,
                              GateRef gate,
                              const JSPandaFile *jsPandaFile = nullptr,
                              const CallMethodFlagMap *callMethodFlagMap = nullptr);
    NO_COPY_SEMANTIC(CallThis1TypeInfoAccessor);
    NO_MOVE_SEMANTIC(CallThis1TypeInfoAccessor);

    GateRef GetArg0() const
    {
        return a0_;
    }

    std::vector<GateRef> GetArgs()
    {
        return { thisObj_, a0_ };
    }

    bool Arg0IsNumberType() const
    {
        return acc_.GetGateType(a0_).IsNumberType();
    }

private:
    GateRef a0_;
};

class CallThis2TypeInfoAccessor final : public CallThisTypeInfoAccessor {
public:
    CallThis2TypeInfoAccessor(const CompilationEnv *env,
                              Circuit *circuit,
                              GateRef gate,
                              const JSPandaFile *jsPandaFile = nullptr,
                              const CallMethodFlagMap *callMethodFlagMap = nullptr);
    NO_COPY_SEMANTIC(CallThis2TypeInfoAccessor);
    NO_MOVE_SEMANTIC(CallThis2TypeInfoAccessor);

    std::vector<GateRef> GetArgs()
    {
        return { thisObj_, a0_, a1_ };
    }
private:
    GateRef a0_;
    GateRef a1_;
};

class CallThis3TypeInfoAccessor final : public CallThisTypeInfoAccessor {
public:
    CallThis3TypeInfoAccessor(const CompilationEnv *env,
                              Circuit *circuit,
                              GateRef gate,
                              const JSPandaFile *jsPandaFile = nullptr,
                              const CallMethodFlagMap *callMethodFlagMap = nullptr);
    NO_COPY_SEMANTIC(CallThis3TypeInfoAccessor);
    NO_MOVE_SEMANTIC(CallThis3TypeInfoAccessor);

    std::vector<GateRef> GetArgs()
    {
        return { thisObj_, a0_, a1_, a2_ };
    }

private:
    GateRef a0_;
    GateRef a1_;
    GateRef a2_;
};

class CallThisRangeTypeInfoAccessor final : public CallThisTypeInfoAccessor {
public:
    CallThisRangeTypeInfoAccessor(const CompilationEnv *env,
                              Circuit *circuit,
                              GateRef gate,
                              const JSPandaFile *jsPandaFile = nullptr,
                              const CallMethodFlagMap *callMethodFlagMap = nullptr);
    NO_COPY_SEMANTIC(CallThisRangeTypeInfoAccessor);
    NO_MOVE_SEMANTIC(CallThisRangeTypeInfoAccessor);
};

enum CallKind : uint8_t {
    CALL,
    CALL_THIS,
    CALL_INIT,
    CALL_SETTER,
    CALL_GETTER,
    SUPER_CALL,
    INVALID
};

class InlineTypeInfoAccessor final : public TypeInfoAccessor {
public:
    InlineTypeInfoAccessor(const CompilationEnv *env,
                           Circuit *circuit,
                           GateRef gate,
                           GateRef receiver,
                           CallKind kind);

    bool IsEnableNormalInline() const
    {
        return IsValidCallMethodId();
    }

    bool IsEnableAccessorInline() const
    {
        if (plr_.IsAccessor()) {
            const PGORWOpType *pgoTypes = acc_.TryGetPGOType(gate_).GetPGORWOpType();
            auto pgoType = pgoTypes->GetObjectInfo(0);
            if (pgoType.GetAccessorMethod().GetProfileType().IsValidCallMethodId()) {
                return true;
            }
        }
        return false;
    }

    bool IsEnableSuperCallInline() const
    {
        return IsValidCallMethodId() && !IsBuiltinFunctionId();
    }

    bool IsBuiltinFunctionId() const
    {
        return pgoType_.GetPGOSampleType()->GetProfileType().IsBuiltinFunctionId();
    }

    bool IsValidCallMethodId() const
    {
        return pgoType_.IsValidCallMethodId();
    }

    uint32_t GetFuncMethodOffsetFromPGO() const
    {
        if (IsValidCallMethodId()) {
            return pgoType_.GetCallMethodId();
        }
        return 0;
    }

    GateType GetReceiverGT() const
    {
        return acc_.GetGateType(receiver_);
    }

    uint32_t GetCallMethodId() const;

    GateRef GetCallGate() const
    {
        return GetGate();
    }

    bool IsCallInit() const
    {
        return kind_ == CallKind::CALL_INIT;
    }

    bool IsCallThis() const
    {
        return kind_ == CallKind::CALL_THIS || kind_ == CallKind::CALL_INIT;
    }

    bool IsNormalCall() const
    {
        return kind_ == CallKind::CALL || kind_ == CallKind::CALL_THIS || kind_ == CallKind::CALL_INIT;
    }

    bool IsCallAccessor() const
    {
        return kind_ == CallKind::CALL_SETTER || kind_ == CallKind::CALL_GETTER;
    }

    bool IsCallGetter() const
    {
        return kind_ == CallKind::CALL_GETTER;
    }

    bool IsCallSetter() const
    {
        return kind_ == CallKind::CALL_SETTER;
    }

    bool IsSuperCall() const
    {
        return kind_ == CallKind::SUPER_CALL;
    }

    uint32_t GetType() const
    {
        return GetFuncMethodOffsetFromPGO();
    }

    PropertyLookupResult GetPlr() const
    {
        return plr_;
    }

    GateRef GetReceiver() const
    {
        return receiver_;
    }

    void UpdateReceiver(GateRef gate)
    {
        receiver_ = gate;
    }

    GateRef GetThisObj() const
    {
        return thisObj_;
    }

    void UpdateThisObj(GateRef gate)
    {
        thisObj_ = gate;
    }

private:
    PropertyLookupResult GetAccessorPlr() const;
    PropertyLookupResult GetAccessorPlrInJIT() const;
    bool InitPropAndCheck(JSTaggedValue &prop) const;

    GateRef receiver_;
    GateRef thisObj_;
    CallKind kind_ {CallKind::INVALID};
    PropertyLookupResult plr_ { PropertyLookupResult() };
};

class ObjectAccessTypeInfoAccessor : public TypeInfoAccessor {
public:
    class ObjectAccessInfo final {
    public:
        explicit ObjectAccessInfo() : plr_(PropertyLookupResult()) {}

        void Set(int hclassIndex, PropertyLookupResult plr)
        {
            hclassIndexes_.emplace_back(hclassIndex);
            plr_ = plr;
        }

        void AppendHClassIndex(int hclassIndex)
        {
            hclassIndexes_.emplace_back(hclassIndex);
        }

        void SetPrimitiveType(PrimitiveType primitiveType)
        {
            primitiveType_ = primitiveType;
        }

        PrimitiveType GetPrimitiveType() const
        {
            return primitiveType_;
        }

        int HClassIndex(size_t index = 0) const
        {
            if (hclassIndexes_.size() <= index) {
                return -1;
            }
            return hclassIndexes_.at(index);
        }

        uint32_t GetData() const
        {
            return plr_.GetData();
        }

        PropertyLookupResult Plr() const
        {
            return plr_;
        }

        const std::vector<int>& GetHClassIndexList() const
        {
            return hclassIndexes_;
        }

    private:
        PropertyLookupResult plr_;
        PrimitiveType primitiveType_ = PrimitiveType::PRIMITIVE_TYPE_INVALID;
        std::vector<int> hclassIndexes_ {};
    };

    enum AccessMode : uint8_t {
        LOAD = 0,
        STORE
    };

    ObjectAccessTypeInfoAccessor(const CompilationEnv *env,
                                 Circuit *circuit,
                                 GateRef gate,
                                 Chunk *chunk,
                                 AccessMode mode)
        : TypeInfoAccessor(env, circuit, gate),
          chunk_(chunk),
          mode_(mode),
          key_(Circuit::NullGate()),
          receiver_(Circuit::NullGate())
    {}
    NO_COPY_SEMANTIC(ObjectAccessTypeInfoAccessor);
    NO_MOVE_SEMANTIC(ObjectAccessTypeInfoAccessor);

    JSTaggedValue GetKeyTaggedValue() const;

    GateRef GetKey() const
    {
        return key_;
    }

    GateRef GetReceiver() const
    {
        return receiver_;
    }

    static bool IsMegaType(const PGORWOpType *pgoTypes)
    {
        for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
            auto temp = pgoTypes->GetObjectInfo(i);
            if (temp.GetReceiverType().IsMegaStateType()) {
                return true;
            }
        }
        return false;
    }

protected:
    Chunk *chunk_;
    AccessMode mode_;
    GateRef key_;
    GateRef receiver_;
};

class ObjAccByNameTypeInfoAccessor : public ObjectAccessTypeInfoAccessor {
public:
    ObjAccByNameTypeInfoAccessor(const CompilationEnv *env,
                                 Circuit *circuit,
                                 GateRef gate,
                                 Chunk *chunk,
                                 AccessMode mode)
        : ObjectAccessTypeInfoAccessor(env, circuit, gate, chunk, mode),
          hasIllegalType_(false),
          accessInfos_(chunk),
          checkerInfos_(chunk)
    {}

    NO_COPY_SEMANTIC(ObjAccByNameTypeInfoAccessor);
    NO_MOVE_SEMANTIC(ObjAccByNameTypeInfoAccessor);

    bool HasIllegalType() const
    {
        return hasIllegalType_;
    }

    int GetExpectedHClassIndex(size_t index) const
    {
        ASSERT(index < checkerInfos_.size());
        return checkerInfos_[index].HClassIndex();
    }

    const std::vector<int>& GetExpectedHClassIndexList(size_t index) const
    {
        ASSERT(index < checkerInfos_.size());
        return checkerInfos_[index].GetHClassIndexList();
    }

    ObjectAccessInfo GetAccessInfo(size_t index) const
    {
        ASSERT(index < accessInfos_.size());
        return accessInfos_[index];
    }

protected:
    bool GeneratePlr(ProfileTyper type, ObjectAccessInfo &info, JSTaggedValue key) const;
    bool GeneratePlrInJIT(JSHClass* hclass, ObjectAccessInfo &info, JSTaggedValue key) const;

    bool hasIllegalType_;
    ChunkVector<ObjectAccessInfo> accessInfos_;
    ChunkVector<ObjectAccessInfo> checkerInfos_;
};

class LoadPrivatePropertyTypeInfoAccessor final : public ObjAccByNameTypeInfoAccessor {
public:
    class AccessorStrategy {
    public:
        virtual ~AccessorStrategy() = default;
        virtual bool TypesIsEmpty() const = 0;
        virtual bool IsMono() const = 0;
        virtual void FetchPGORWTypesDual() = 0;
        virtual bool GenerateObjectAccessInfo() = 0;
    };

    class AotAccessorStrategy : public AccessorStrategy {
    public:
        explicit AotAccessorStrategy(LoadPrivatePropertyTypeInfoAccessor &parent) : parent_(parent)
        {
        }

        bool TypesIsEmpty() const override
        {
            return parent_.types_.empty();
        }

        bool IsMono() const override
        {
            return parent_.accessInfos_.size() == 1;
        }

        void FetchPGORWTypesDual() override;
        bool GenerateObjectAccessInfo() override;

    private:
        LoadPrivatePropertyTypeInfoAccessor &parent_;
    };

    class JitAccessorStrategy : public AccessorStrategy {
    public:
        explicit JitAccessorStrategy(LoadPrivatePropertyTypeInfoAccessor &parent) : parent_(parent)
        {
        }

        bool TypesIsEmpty() const override
        {
            return parent_.jitTypes_.empty();
        }

        bool IsMono() const override
        {
            return parent_.jitTypes_.size() == 1;
        }

        void FetchPGORWTypesDual() override;
        bool GenerateObjectAccessInfo() override;

    private:
        LoadPrivatePropertyTypeInfoAccessor &parent_;
    };

    LoadPrivatePropertyTypeInfoAccessor(const CompilationEnv *env, Circuit *circuit, GateRef gate, Chunk *chunk)
        : ObjAccByNameTypeInfoAccessor(env, circuit, gate, chunk, AccessMode::STORE), types_(chunk_), jitTypes_(chunk_)
    {
        levelIndex_ = acc_.GetValueIn(gate, 1); // 1: levelIndex
        slotIndex_ = acc_.GetValueIn(gate, 2);  // 2: slotIndex
        lexicalEnv_ = acc_.GetValueIn(gate, 3); // 3: lexicalEnv
        receiver_ = acc_.GetValueIn(gate, 4);   // 4: acc as receiver
        if (IsAot()) {
            strategy_ = chunk_->New<AotAccessorStrategy>(*this);
        } else {
            strategy_ = chunk_->New<JitAccessorStrategy>(*this);
        }
        strategy_->FetchPGORWTypesDual();
        hasIllegalType_ = !strategy_->GenerateObjectAccessInfo();
    }
    NO_COPY_SEMANTIC(LoadPrivatePropertyTypeInfoAccessor);
    NO_MOVE_SEMANTIC(LoadPrivatePropertyTypeInfoAccessor);

    bool IsMono() const
    {
        return strategy_->IsMono();
    }

    bool TypesIsEmpty() const
    {
        return strategy_->TypesIsEmpty();
    }

    GateRef GetLevelIndex() const
    {
        return levelIndex_;
    }

    GateRef GetSlotIndex() const
    {
        return slotIndex_;
    }

    GateRef GetLexicalEnv() const
    {
        return lexicalEnv_;
    }

    bool IsAccessor() const
    {
        return isAccessor_;
    }

private:
    void FetchPGORWTypesDual();
    bool GenerateObjectAccessInfo();

    void FetchPGORWTypesDualInJIT();
    bool GenerateObjectAccessInfoInJIT();
    JSTaggedValue GetKeyTaggedValue() const;

    ChunkVector<std::pair<ProfileTyper, ProfileTyper>> types_;
    ChunkVector<pgo::PGOObjectInfo> jitTypes_;
    GateRef levelIndex_;
    GateRef slotIndex_;
    GateRef lexicalEnv_;
    bool isAccessor_{false};
    AccessorStrategy* strategy_;
    friend class AotAccessorStrategy;
    friend class JitAccessorStrategy;
};

class StorePrivatePropertyTypeInfoAccessor final : public ObjAccByNameTypeInfoAccessor {
public:
    class AccessorStrategy {
    public:
        virtual ~AccessorStrategy() = default;
        virtual bool TypesIsEmpty() const = 0;
        virtual bool IsMono() const = 0;
        virtual void FetchPGORWTypesDual() = 0;
        virtual bool GenerateObjectAccessInfo() = 0;
    };

    class AotAccessorStrategy : public AccessorStrategy {
    public:
        explicit AotAccessorStrategy(StorePrivatePropertyTypeInfoAccessor &parent) : parent_(parent)
        {
        }

        bool TypesIsEmpty() const override
        {
            return parent_.types_.empty();
        }

        bool IsMono() const override
        {
            return parent_.types_.size() == 1;
        }

        void FetchPGORWTypesDual() override;
        bool GenerateObjectAccessInfo() override;

    private:
        StorePrivatePropertyTypeInfoAccessor &parent_;
    };

    class JitAccessorStrategy : public AccessorStrategy {
    public:
        explicit JitAccessorStrategy(StorePrivatePropertyTypeInfoAccessor &parent) : parent_(parent)
        {
        }

        bool TypesIsEmpty() const override
        {
            return parent_.jitTypes_.empty();
        }

        bool IsMono() const override
        {
            return parent_.jitTypes_.size() == 1;
        }

        void FetchPGORWTypesDual() override;
        bool GenerateObjectAccessInfo() override;

    private:
        StorePrivatePropertyTypeInfoAccessor &parent_;
    };

    StorePrivatePropertyTypeInfoAccessor(const CompilationEnv *env, Circuit *circuit, GateRef gate, Chunk *chunk)
        : ObjAccByNameTypeInfoAccessor(env, circuit, gate, chunk, AccessMode::STORE), types_(chunk_), jitTypes_(chunk)
    {
        levelIndex_ = acc_.GetValueIn(gate, 1); // 1: levelIndex
        slotIndex_ = acc_.GetValueIn(gate, 2);  // 2: slotIndex
        receiver_ = acc_.GetValueIn(gate, 3);   // 3: receiver
        lexicalEnv_ = acc_.GetValueIn(gate, 4); // 4: lexicalEnv
        value_ = acc_.GetValueIn(gate, 5);      // 5: acc as value
        if (IsAot()) {
            strategy_ = chunk_->New<AotAccessorStrategy>(*this);
        } else {
            strategy_ = chunk_->New<JitAccessorStrategy>(*this);
        }
        strategy_->FetchPGORWTypesDual();
        hasIllegalType_ = !strategy_->GenerateObjectAccessInfo();
    }
    NO_COPY_SEMANTIC(StorePrivatePropertyTypeInfoAccessor);
    NO_MOVE_SEMANTIC(StorePrivatePropertyTypeInfoAccessor);

    bool IsMono() const
    {
        return strategy_->IsMono();
    }

    bool TypesIsEmpty() const
    {
        return strategy_->TypesIsEmpty();
    }

    GateRef GetValue() const
    {
        return value_;
    }

    GateRef GetLevelIndex() const
    {
        return levelIndex_;
    }

    GateRef GetSlotIndex() const
    {
        return slotIndex_;
    }

    GateRef GetLexicalEnv() const
    {
        return lexicalEnv_;
    }

    bool IsAccessor() const
    {
        return isAccessor_;
    }

private:
    JSTaggedValue GetKeyTaggedValue() const;

    ChunkVector<std::tuple<ProfileTyper, ProfileTyper, ProfileTyper>> types_;
    ChunkVector<pgo::PGOObjectInfo> jitTypes_;
    GateRef value_;
    GateRef levelIndex_;
    GateRef slotIndex_;
    GateRef lexicalEnv_;
    bool isAccessor_{false};
    AccessorStrategy* strategy_;
    friend class AotAccessorStrategy;
    friend class JitAccessorStrategy;
};

class LoadObjPropertyTypeInfoAccessor final : public ObjAccByNameTypeInfoAccessor {
public:
    class AccessorStrategy {
    public:
        virtual ~AccessorStrategy() = default;
        virtual size_t GetTypeCount() const = 0;
        virtual bool TypesIsEmpty() const = 0;
        virtual bool IsMono() const = 0;
        virtual bool IsReceiverEqHolder(size_t index) const = 0;
        virtual void FetchPGORWTypesDual() = 0;
        virtual bool GenerateObjectAccessInfo() = 0;
        virtual JSHClass* GetReceiverHClass(size_t index) const = 0;
        virtual JSHClass* GetHolderHClass(size_t index) const = 0;
    };

    class AotAccessorStrategy : public AccessorStrategy {
    public:
        explicit AotAccessorStrategy(LoadObjPropertyTypeInfoAccessor &parent) : parent_(parent)
        {
        }

        size_t GetTypeCount() const override
        {
            return parent_.accessInfos_.size();
        }

        bool TypesIsEmpty() const override
        {
            return parent_.types_.empty();
        }

        bool IsMono() const override
        {
            return parent_.accessInfos_.size() == 1;
        }

        bool IsReceiverEqHolder(size_t index) const override
        {
            ASSERT(index < parent_.types_.size());
            return parent_.types_[index].first == parent_.types_[index].second;
        }

        JSHClass* GetReceiverHClass([[maybe_unused]] size_t index) const override
        {
            LOG_FULL(FATAL) << "Aot should not get receiver hclass";
            UNREACHABLE();
        }

        JSHClass* GetHolderHClass([[maybe_unused]] size_t index) const override
        {
            LOG_FULL(FATAL) << "Aot should not get holder hclass";
            UNREACHABLE();
        }

        void FetchPGORWTypesDual() override;
        bool GenerateObjectAccessInfo() override;

    private:
        LoadObjPropertyTypeInfoAccessor &parent_;
    };

    class JitAccessorStrategy : public AccessorStrategy {
    public:
        explicit JitAccessorStrategy(LoadObjPropertyTypeInfoAccessor &parent) : parent_(parent)
        {
        }

        size_t GetTypeCount() const override
        {
            return parent_.accessInfos_.size();
        }

        bool TypesIsEmpty() const override
        {
            return parent_.accessInfos_.empty();
        }

        bool IsMono() const override
        {
            return parent_.accessInfos_.size() == 1;
        }

        bool IsReceiverEqHolder(size_t index) const override
        {
            return parent_.jitTypes_[index].GetReceiverHclass() == parent_.jitTypes_[index].GetHolderHclass();
        }

        JSHClass* GetReceiverHClass(size_t index) const override
        {
            return parent_.jitTypes_[index].GetReceiverHclass();
        }

        JSHClass* GetHolderHClass(size_t index) const override
        {
            return parent_.jitTypes_[index].GetHolderHclass();
        }

        void FetchPGORWTypesDual() override;
        bool GenerateObjectAccessInfo() override;

    private:
        LoadObjPropertyTypeInfoAccessor &parent_;
    };
    LoadObjPropertyTypeInfoAccessor(const CompilationEnv *env, Circuit *circuit,
                                    GateRef gate, Chunk *chunk, bool isByValue = false);
    NO_COPY_SEMANTIC(LoadObjPropertyTypeInfoAccessor);
    NO_MOVE_SEMANTIC(LoadObjPropertyTypeInfoAccessor);

    size_t GetTypeCount() const
    {
        return strategy_->GetTypeCount();
    }

    bool TypesIsEmpty()
    {
        return strategy_->TypesIsEmpty();
    }

    bool IsMono()
    {
        return strategy_->IsMono();
    }

    PrimitiveType GetPrimitiveType(size_t index) const
    {
        return accessInfos_.at(index).GetPrimitiveType();
    }

    bool IsReceiverEqHolder(size_t index) const
    {
        return accessInfos_.at(index).HClassIndex() == checkerInfos_.at(index).HClassIndex();
    }

    bool CanBeMerged(ObjectAccessInfo& leftAccessInfo, ObjectAccessInfo& leftCheckerInfo,
                     ObjectAccessInfo& rightAccessInfo, ObjectAccessInfo& rightCheckerInfo)
    {
        if (leftAccessInfo.GetPrimitiveType() != PrimitiveType::PRIMITIVE_TYPE_INVALID ||
            rightAccessInfo.GetPrimitiveType() != PrimitiveType::PRIMITIVE_TYPE_INVALID ||
            leftCheckerInfo.GetPrimitiveType() != PrimitiveType::PRIMITIVE_TYPE_INVALID ||
            rightCheckerInfo.GetPrimitiveType() != PrimitiveType::PRIMITIVE_TYPE_INVALID) {
            return false;
        }
        if (leftCheckerInfo.HClassIndex() == leftAccessInfo.HClassIndex()) {
            if (rightAccessInfo.HClassIndex() == rightCheckerInfo.HClassIndex() &&
                rightAccessInfo.GetData() == leftAccessInfo.GetData()) {
                return true;
            }
            return false;
        } else {
            if (leftAccessInfo.HClassIndex() == rightAccessInfo.HClassIndex() &&
                rightAccessInfo.GetData() == leftAccessInfo.GetData()) {
                return true;
            }
            return false;
        }
    }

    void TryMergeExpectedHClass()
    {
        std::vector<bool> infoIsMerged(accessInfos_.size(), false);
        for (size_t i = 0; i < accessInfos_.size(); i++) {
            ObjectAccessInfo& accessInfo = accessInfos_[i];
            ObjectAccessInfo& checkerInfo = checkerInfos_[i];
            bool isMerged = false;

            for (auto j = 0; j < i; j++) {
                if (infoIsMerged[j]) {
                    continue;
                }
                if (CanBeMerged(accessInfo, checkerInfo, accessInfos_[j], checkerInfos_[j])) {
                    isMerged = true;
                    accessInfos_[j].AppendHClassIndex(accessInfo.HClassIndex());
                    checkerInfos_[j].AppendHClassIndex(checkerInfo.HClassIndex());
                    infoIsMerged[i] = true;
                    break;
                }
            }
            infoIsMerged.at(i) = isMerged;
        }
        for (int i = accessInfos_.size() - 1; i >= 0; i--) {
            if (infoIsMerged[i]) {
                accessInfos_.erase(accessInfos_.begin() + i);
                checkerInfos_.erase(checkerInfos_.begin() + i);
            }
        }
    }

    JSHClass* GetReceiverHClass(size_t index) const
    {
        return strategy_->GetReceiverHClass(index);
    }

    JSHClass* GetHolderHClass(size_t index) const
    {
        return strategy_->GetHolderHClass(index);
    }

    JSHandle<JSTaggedValue> GetName()
    {
        return name_;
    }

    uint32_t GetNameIdx()
    {
        return nameIdx_;
    }

    std::map<size_t, uint32_t> CollectPrimitiveTypeInfo(const CompilationEnv *compilationEnv) const
    {
        std::map <size_t, uint32_t> typeIndex2HeapConstantIndex;
        if (compilationEnv == nullptr || !compilationEnv->SupportHeapConstant()) {
            return typeIndex2HeapConstantIndex;
        }
        size_t typeCount = GetTypeCount();
        auto *jitCompilationEnv = static_cast<const JitCompilationEnv*>(compilationEnv);
        const auto &holderHClassIndex2HeapConstantIndex = jitCompilationEnv->GetHolderHClassIndex2HeapConstantIndex();
        for (size_t i = 0; i < typeCount; ++i) {
            auto primitiveType = GetPrimitiveType(i);
            if (primitiveType != PrimitiveType::PRIMITIVE_NUMBER && primitiveType != PrimitiveType::PRIMITIVE_BOOLEAN) {
                continue;
            }
            ObjectAccessTypeInfoAccessor::ObjectAccessInfo info = GetAccessInfo(i);
            auto holderHClassIndex = info.HClassIndex();
            auto itr = holderHClassIndex2HeapConstantIndex.find(holderHClassIndex);
            if (itr != holderHClassIndex2HeapConstantIndex.end()) {
                typeIndex2HeapConstantIndex.insert(std::pair<size_t, uint32_t>(i, itr->second));
            }
        }
        return typeIndex2HeapConstantIndex;
    }

private:
    ChunkVector<std::pair<ProfileTyper, ProfileTyper>> types_;
    ChunkVector<pgo::PGOObjectInfo> jitTypes_;
    JSHandle<JSTaggedValue> name_;
    uint32_t nameIdx_;
    bool isByValue_;

    AccessorStrategy* strategy_;
    friend class AotAccessorStrategy;
    friend class JitAccessorStrategy;
};

class StoreObjByNameTypeInfoAccessor final : public ObjAccByNameTypeInfoAccessor {
public:
    class AccessorStrategy {
    public:
        virtual ~AccessorStrategy() = default;
        virtual size_t GetTypeCount() const = 0;
        virtual bool TypesIsEmpty() const = 0;
        virtual bool IsMono() const = 0;
        virtual bool IsReceiverEqHolder(size_t index) const = 0;
        virtual bool IsReceiverNoEqNewHolder(size_t index) const = 0;
        virtual bool IsHolderEqNewHolder(size_t index) const = 0;
        virtual void FetchPGORWTypesDual() = 0;
        virtual bool GenerateObjectAccessInfo() = 0;
        virtual JSHClass* GetReceiverHClass(size_t index) const = 0;
        virtual JSHClass* GetHolderHClass(size_t index) const = 0;
        virtual bool IsPrototypeHclass(size_t index) const = 0;
    };

    class AotAccessorStrategy : public AccessorStrategy {
    public:
        explicit AotAccessorStrategy(StoreObjByNameTypeInfoAccessor &parent) : parent_(parent)
        {
        }

        size_t GetTypeCount() const override
        {
            return parent_.types_.size();
        }

        bool TypesIsEmpty() const override
        {
            return parent_.types_.empty();
        }

        bool IsMono() const override
        {
            return parent_.types_.size() == 1;
        }

        bool IsReceiverEqHolder(size_t index) const override
        {
            return std::get<HclassIndex::Reciver>(parent_.types_[index]) ==
                   std::get<HclassIndex::Holder>(parent_.types_[index]);
        }

        bool IsReceiverNoEqNewHolder(size_t index) const override
        {
            return std::get<HclassIndex::Reciver>(parent_.types_[index]) !=
                   std::get<HclassIndex::HolderTra>(parent_.types_[index]);
        }

        bool IsHolderEqNewHolder(size_t index) const override
        {
            return std::get<HclassIndex::Holder>(parent_.types_[index]) ==
                   std::get<HclassIndex::HolderTra>(parent_.types_[index]);
        }

        bool IsPrototypeHclass(size_t index) const override
        {
            ProfileTyper recv = std::get<HclassIndex::Reciver>(parent_.types_[index]);
            JSTaggedValue hclass = parent_.ptManager_->QueryHClass(recv.first, recv.second);
            if (!hclass.IsJSHClass()) {
                return false;
            }
            return JSHClass::Cast(hclass.GetTaggedObject())->IsPrototype();
        }

        JSHClass* GetReceiverHClass([[maybe_unused]] size_t index) const override
        {
            LOG_FULL(FATAL) << "Aot should not get receiver hclass";
            UNREACHABLE();
        }

        JSHClass* GetHolderHClass([[maybe_unused]] size_t index) const override
        {
            LOG_FULL(FATAL) << "Aot should not get holder hclass";
            UNREACHABLE();
        }

        void FetchPGORWTypesDual() override;
        bool GenerateObjectAccessInfo() override;

    private:
        StoreObjByNameTypeInfoAccessor &parent_;
    };

    class JitAccessorStrategy : public AccessorStrategy {
    public:
        explicit JitAccessorStrategy(StoreObjByNameTypeInfoAccessor &parent) : parent_(parent)
        {
        }

        size_t GetTypeCount() const override
        {
            return parent_.jitTypes_.size();
        }

        bool TypesIsEmpty() const override
        {
            return parent_.jitTypes_.empty();
        }

        bool IsMono() const override
        {
            return parent_.jitTypes_.size() == 1;
        }

        bool IsReceiverEqHolder(size_t index) const override
        {
            return parent_.jitTypes_[index].GetReceiverHclass() == parent_.jitTypes_[index].GetHolderHclass();
        }

        bool IsReceiverNoEqNewHolder(size_t index) const override
        {
            return parent_.jitTypes_[index].GetReceiverHclass() != parent_.jitTypes_[index].GetHolderTraHclass();
        }

        bool IsHolderEqNewHolder(size_t index) const override
        {
            return parent_.jitTypes_[index].GetHolderHclass() == parent_.jitTypes_[index].GetHolderTraHclass();
        }

        bool IsPrototypeHclass(size_t index) const override
        {
            return parent_.jitTypes_[index].GetReceiverHclass()->IsPrototype();
        }

        JSHClass* GetReceiverHClass(size_t index) const override
        {
            return parent_.jitTypes_[index].GetReceiverHclass();
        }

        JSHClass* GetHolderHClass(size_t index) const override
        {
            return parent_.jitTypes_[index].GetHolderHclass();
        }

        void FetchPGORWTypesDual() override;
        bool GenerateObjectAccessInfo() override;

    private:
        StoreObjByNameTypeInfoAccessor &parent_;
    };

    StoreObjByNameTypeInfoAccessor(const CompilationEnv *env, Circuit *circuit, GateRef gate, Chunk *chunk);
    NO_COPY_SEMANTIC(StoreObjByNameTypeInfoAccessor);
    NO_MOVE_SEMANTIC(StoreObjByNameTypeInfoAccessor);

    size_t GetTypeCount()
    {
        return strategy_->GetTypeCount();
    }

    bool TypesIsEmpty()
    {
        return strategy_->TypesIsEmpty();
    }

    bool IsMono()
    {
        return strategy_->IsMono();
    }

    bool IsReceiverEqHolder(size_t index) const
    {
        return strategy_->IsReceiverEqHolder(index);
    }

    bool IsReceiverNoEqNewHolder(size_t index) const
    {
        return strategy_->IsReceiverNoEqNewHolder(index);
    }

    bool IsHolderEqNewHolder(size_t index) const
    {
        return strategy_->IsHolderEqNewHolder(index);
    }

    bool IsPrototypeHclass(size_t index) const
    {
        return strategy_->IsPrototypeHclass(index);
    }

    JSHClass* GetReceiverHClass(size_t index) const
    {
        return strategy_->GetReceiverHClass(index);
    }

    JSHClass* GetHolderHClass(size_t index) const
    {
        return strategy_->GetHolderHClass(index);
    }

    GateRef GetValue() const
    {
        return value_;
    }

private:
    enum HclassIndex {
        Reciver = 0,
        Holder,
        HolderTra
    };
    ChunkVector<std::tuple<ProfileTyper, ProfileTyper, ProfileTyper>> types_;
    ChunkVector<pgo::PGOObjectInfo> jitTypes_;
    GateRef value_;
    AccessorStrategy* strategy_;
    friend class AotAccessorStrategy;
    friend class JitAccessorStrategy;
};

class InstanceOfTypeInfoAccessor final : public ObjAccByNameTypeInfoAccessor {
public:
    class AccessorStrategy {
    public:
        virtual ~AccessorStrategy() = default;
        virtual size_t GetTypeCount() const = 0;
        virtual bool TypesIsEmpty() const = 0;
        virtual bool IsMono() const = 0;
        virtual void FetchPGORWTypesDual() = 0;
        virtual bool GenerateObjectAccessInfo() = 0;
        virtual bool ClassInstanceIsCallable(ProfileTyper type) const = 0;
        virtual bool ClassInstanceIsCallable(JSHClass *hclass) const = 0;
    };

    class AotAccessorStrategy : public AccessorStrategy {
    public:
        explicit AotAccessorStrategy(InstanceOfTypeInfoAccessor &parent) : parent_(parent)
        {
        }

        size_t GetTypeCount() const override
        {
            return parent_.types_.size();
        }

        bool TypesIsEmpty() const override
        {
            return parent_.types_.empty();
        }

        bool IsMono() const override
        {
            return parent_.types_.size() == 1;
        }

        void FetchPGORWTypesDual() override;
        bool GenerateObjectAccessInfo() override;
        bool ClassInstanceIsCallable(ProfileTyper type) const override;
        bool ClassInstanceIsCallable([[maybe_unused]] JSHClass *hclass) const override
        {
            ASSERT(0);
            return false;
        }

    private:
        InstanceOfTypeInfoAccessor &parent_;
    };

    class JitAccessorStrategy : public AccessorStrategy {
    public:
        explicit JitAccessorStrategy(InstanceOfTypeInfoAccessor &parent) : parent_(parent)
        {
        }

        size_t GetTypeCount() const override
        {
            return parent_.jitTypes_.size();
        }

        bool TypesIsEmpty() const override
        {
            return parent_.jitTypes_.empty();
        }

        bool IsMono() const override
        {
            return parent_.jitTypes_.size() == 1;
        }
        void FetchPGORWTypesDual() override;
        bool GenerateObjectAccessInfo() override;
        bool ClassInstanceIsCallable(JSHClass *hclass) const override;
        bool ClassInstanceIsCallable([[maybe_unused]] ProfileTyper type) const override
        {
            ASSERT(0);
            return false;
        }

    private:
        InstanceOfTypeInfoAccessor &parent_;
    };

    InstanceOfTypeInfoAccessor(const CompilationEnv *env, Circuit *circuit, GateRef gate, Chunk *chunk);
    NO_COPY_SEMANTIC(InstanceOfTypeInfoAccessor);
    NO_MOVE_SEMANTIC(InstanceOfTypeInfoAccessor);

    size_t GetTypeCount()
    {
        return strategy_->GetTypeCount();
    }

    bool TypesIsEmpty()
    {
        return strategy_->TypesIsEmpty();
    }

    bool IsMono()
    {
        return strategy_->IsMono();
    }

    JSTaggedValue GetKeyTaggedValue() const;

    GateRef GetTarget() const
    {
        return target_;
    }

private:
    ChunkVector<std::pair<ProfileTyper, ProfileTyper>> types_;
    ChunkVector<pgo::PGOObjectInfo> jitTypes_;
    GateRef target_;
    AccessorStrategy* strategy_;

    friend class AotAccessorStrategy;
    friend class JitAccessorStrategy;
};

class AccBuiltinObjTypeInfoAccessor : public ObjectAccessTypeInfoAccessor {
public:
    AccBuiltinObjTypeInfoAccessor(const CompilationEnv *env,
                                  Circuit *circuit,
                                  GateRef gate,
                                  Chunk *chunk,
                                  AccessMode mode)
        : ObjectAccessTypeInfoAccessor(env, circuit, gate, chunk, mode), types_(chunk_)
    {}
    NO_COPY_SEMANTIC(AccBuiltinObjTypeInfoAccessor);
    NO_MOVE_SEMANTIC(AccBuiltinObjTypeInfoAccessor);

    bool IsMono() const
    {
        return types_.size() == 1 || IsMonoBuiltins();
    }

    size_t GetTypeCount()
    {
        return types_.size();
    }

    bool IsBuiltinsMap() const
    {
        return types_[0].IsBuiltinsMap();
    }

    bool IsBuiltinsString() const
    {
        return types_[0].IsBuiltinsString();
    }

    bool IsBuiltinsArray() const
    {
        return types_[0].IsBuiltinsArray();
    }

    bool IsBuiltinsTypeArray() const
    {
        return types_[0].IsBuiltinsTypeArray();
    }

    bool IsStoreOutOfBounds() const
    {
        ASSERT(types_.size() > 0);
        return types_[0].IsEverOutOfBounds();
    }

    bool HasNoType() const
    {
        return types_.empty();
    }

    JSType GetBuiltinsJSType() const
    {
        if (types_[0].IsBuiltinsType()) {
            return types_[0].GetBuiltinsType();
        }
        return JSType::INVALID;
    }

    ParamType GetParamType() const
    {
        ASSERT(IsMono());
        return TypeInfoAccessor::PGOBuiltinTypeToParamType(types_[0]);
    }

    bool IsPolyBuiltinsArray() const
    {
        if (types_.size() == 0) {
            return false;
        }
        for (size_t i = 0; i < types_.size(); ++i) {
            if (!types_[i].IsBuiltinsArray()) {
                return false;
            }
        }
        return true;
    }

    ElementsKind GetElementsKindBeforeTransition(size_t index)
    {
        ProfileType currType = types_[index];
        return currType.GetElementsKindBeforeTransition();
    }

    ElementsKind GetElementsKindAfterTransition(size_t index)
    {
        ProfileType currType = types_[index];
        return currType.GetElementsKindAfterTransition();
    }

    bool IsBuiltinsType() const
    {
        return IsMono() && types_[0].IsBuiltinsType();
    }

    bool IsGlobalsType() const
    {
        return IsMono() && types_[0].IsGlobalsType();
    }

    std::optional<BuiltinTypeId> GetBuiltinsTypeId() const
    {
        if (!IsMono()) {
            return std::nullopt;
        }
        auto type = types_[0].GetBuiltinsType();
        return ToBuiltinsTypeId(type);
    }

    std::optional<GlobalIndex> GetGlobalsId() const
    {
        return types_[0].GetGlobalsId();
    }

    OnHeapMode TryGetHeapMode() const
    {
        return acc_.TryGetOnHeapMode(gate_);
    }

    uint32_t TryConvertKeyToInt() const
    {
        return static_cast<uint32_t>(acc_.GetConstantValue(GetKey()));
    }

    // Default get is elementsKind before possible transition
    ElementsKind TryGetArrayElementsKind() const
    {
        [[maybe_unused]] bool condition = (IsMono() && IsBuiltinsArray());
        ASSERT(condition);
        return acc_.TryGetArrayElementsKind(gate_);
    }

    ElementsKind TryGetArrayElementsKindAfterTransition() const
    {
        [[maybe_unused]] bool condition = (IsMono() && IsBuiltinsArray());
        ASSERT(condition);
        return acc_.TryGetArrayElementsKindAfterTransition(gate_);
    }

protected:
    bool IsMonoBuiltins() const;
    bool IsStringMonoBuiltins() const;
    void FetchBuiltinsTypes();
    bool CheckDuplicatedBuiltinType(ProfileType newType) const;

    ChunkVector<ProfileType> types_;
};

class LoadBulitinObjTypeInfoAccessor final : public AccBuiltinObjTypeInfoAccessor {
public:
    LoadBulitinObjTypeInfoAccessor(const CompilationEnv *env,
                                   Circuit *circuit,
                                   GateRef gate,
                                   Chunk *chunk);
    NO_COPY_SEMANTIC(LoadBulitinObjTypeInfoAccessor);
    NO_MOVE_SEMANTIC(LoadBulitinObjTypeInfoAccessor);
};

class StoreBulitinObjTypeInfoAccessor final : public AccBuiltinObjTypeInfoAccessor {
public:
    StoreBulitinObjTypeInfoAccessor(const CompilationEnv *env,
                                    Circuit *circuit,
                                    GateRef gate,
                                    Chunk *chunk);
    NO_COPY_SEMANTIC(StoreBulitinObjTypeInfoAccessor);
    NO_MOVE_SEMANTIC(StoreBulitinObjTypeInfoAccessor);

    bool ValueIsNumberType() const
    {
        return acc_.GetGateType(value_).IsNumberType();
    }

    GateRef GetValue() const
    {
        return value_;
    }

private:
    GateRef value_ {Circuit::NullGate()};
};

class GlobalObjAccTypeInfoAccessor : public ObjectAccessTypeInfoAccessor {
public:
    GlobalObjAccTypeInfoAccessor(const CompilationEnv *env,
                                 Circuit *circuit,
                                 GateRef gate,
                                 AccessMode mode)
        : ObjectAccessTypeInfoAccessor(env, circuit, gate, nullptr, mode) {}

    NO_COPY_SEMANTIC(GlobalObjAccTypeInfoAccessor);
    NO_MOVE_SEMANTIC(GlobalObjAccTypeInfoAccessor);
};

class LoadGlobalObjByNameTypeInfoAccessor final : public GlobalObjAccTypeInfoAccessor {
public:
    LoadGlobalObjByNameTypeInfoAccessor(const CompilationEnv *env,
                                        Circuit *circuit,
                                        GateRef gate)
        : GlobalObjAccTypeInfoAccessor(env, circuit, gate, AccessMode::LOAD)
    {
        key_ = acc_.GetValueIn(gate, 1);
    }
    NO_COPY_SEMANTIC(LoadGlobalObjByNameTypeInfoAccessor);
    NO_MOVE_SEMANTIC(LoadGlobalObjByNameTypeInfoAccessor);
};

class CreateObjWithBufferTypeInfoAccessor : public TypeInfoAccessor {
public:
    CreateObjWithBufferTypeInfoAccessor(const CompilationEnv *env,
                                        Circuit *circuit,
                                        GateRef gate,
                                        const CString &recordName,
                                        Chunk* chunk);

    NO_COPY_SEMANTIC(CreateObjWithBufferTypeInfoAccessor);
    NO_MOVE_SEMANTIC(CreateObjWithBufferTypeInfoAccessor);

    class AccessorStrategy {
    public:
        virtual ~AccessorStrategy() = default;
        virtual JSTaggedValue GetHClass() const = 0;
    };

    class AotAccessorStrategy : public AccessorStrategy {
    public:
        explicit AotAccessorStrategy(CreateObjWithBufferTypeInfoAccessor &parent) : parent_(parent)
        {
        }

        JSTaggedValue GetHClass() const override;

    private:
        CreateObjWithBufferTypeInfoAccessor &parent_;
    };

    class JitAccessorStrategy : public AccessorStrategy {
    public:
        explicit JitAccessorStrategy(CreateObjWithBufferTypeInfoAccessor &parent) : parent_(parent)
        {
        }

        JSTaggedValue GetHClass() const override;

    private:
        CreateObjWithBufferTypeInfoAccessor &parent_;
    };

    JSTaggedValue GetHClass() const
    {
        return strategy_->GetHClass();
    }

    JSTaggedValue GetObject() const;

    GateRef GetIndex() const
    {
        return index_;
    }

    bool CanOptimize() const
    {
        JSTaggedValue obj = GetObject();
        if (obj.IsUndefined()) {
            return false;
        }
        JSObject *jsObj = JSObject::Cast(obj);
        TaggedArray *properties = TaggedArray::Cast(jsObj->GetProperties());
        TaggedArray *elements = TaggedArray::Cast(jsObj->GetElements());
        return properties->GetLength() == 0 && elements->GetLength() == 0;
    }

private:
    void Init();

    const CString &recordName_;
    GateRef index_;
    AccessorStrategy* strategy_;

    friend class AotAccessorStrategy;
    friend class JitAccessorStrategy;
};
}   // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TYPE_INFO_ACCESSORS_H
