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
#include "ecmascript/enum_conversion.h"
#include "ecmascript/global_index.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/ts_types/ts_manager.h"
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
        tsManager_ = compilationEnv_->GetTSManager();
        ptManager_ = compilationEnv_->GetPTManager();
    }

    inline GateRef GetGate() const
    {
        return gate_;
    }

    static bool IsTrustedBooleanType(GateAccessor acc, GateRef gate);

    static bool IsTrustedNumberType(GateAccessor acc, GateRef gate);

    static bool IsTrustedStringType(
        const CompilationEnv *env, Circuit *circuit, Chunk *chunk, GateAccessor acc, GateRef gate);

protected:
    ParamType PGOSampleTypeToParamType() const;
    static ParamType PGOBuiltinTypeToParamType(ProfileType pgoType);

    const CompilationEnv *compilationEnv_ {nullptr};
    GateAccessor acc_;
    ArgumentAccessor argAcc_;
    GateRef gate_;
    PGOTypeRef pgoType_;
    // NOTICE-PGO: wx delete in part3
    TSManager *tsManager_ {nullptr};
    PGOTypeManager *ptManager_ {nullptr};
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
        return pgoType_.HasNumber();
    }

    inline bool IsStringType() const
    {
        return pgoType_.IsString();
    }

    inline bool LeftOrRightIsUndefinedOrNull() const
    {
        return acc_.IsUndefinedOrNullOrHole(left_) || acc_.IsUndefinedOrNullOrHole(right_);
    }

    inline ParamType GetParamType() const
    {
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

    inline ParamType GetParamType() const
    {
        return PGOSampleTypeToParamType();
    }

    GateType GetValueGateType() const
    {
        return acc_.GetGateType(value_);
    }

protected:
    GateRef value_;
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
    NewObjRangeTypeInfoAccessor(const CompilationEnv *env,
                                Circuit *circuit,
                                GateRef gate)
        : UnOpTypeInfoAccessor(env, circuit, gate), hclassIndex_(-1) {}
    NO_COPY_SEMANTIC(NewObjRangeTypeInfoAccessor);
    NO_MOVE_SEMANTIC(NewObjRangeTypeInfoAccessor);

    bool FindHClass();

    int GetHClassIndex() const
    {
        return hclassIndex_;
    }

private:
    int hclassIndex_;
};

class NewBuiltinCtorTypeInfoAccessor final : public UnOpTypeInfoAccessor {
public:
    NewBuiltinCtorTypeInfoAccessor(const CompilationEnv *env,
                                   Circuit *circuit,
                                   GateRef gate)
        : UnOpTypeInfoAccessor(env, circuit, gate) {}
    NO_COPY_SEMANTIC(NewBuiltinCtorTypeInfoAccessor);
    NO_MOVE_SEMANTIC(NewBuiltinCtorTypeInfoAccessor);

    bool IsBuiltinModule() const
    {
        return GetCtorGT().IsBuiltinModule();
    }

    bool IsBuiltinConstructor(BuiltinTypeId type)
    {
        if (compilationEnv_->IsJitCompiler()) {
            return false;
        }
        return tsManager_->IsBuiltinConstructor(type, GetCtorGT());
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
                              GateRef gate);
    NO_COPY_SEMANTIC(SuperCallTypeInfoAccessor);
    NO_MOVE_SEMANTIC(SuperCallTypeInfoAccessor);

    bool IsClassTypeKind() const
    {
        if (compilationEnv_->IsJitCompiler()) {
            return false;
        }
        return tsManager_->IsClassTypeKind(acc_.GetGateType(ctor_));
    }

    bool IsValidCallMethodId() const
    {
        return pgoType_.IsValidCallMethodId();
    }

    GateRef GetCtor() const
    {
        return ctor_;
    }

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
            return false;
        }
        auto profileType = acc_.TryGetPGOType(gate_).GetPGOSampleType();
        bool haveProfileType = !profileType->IsNone();
        if (haveProfileType) {
            uint32_t methodId = profileType->GetProfileType().GetId();
            MethodLiteral *targetMethodLiteral = jsPandaFile_->FindMethodLiteral(methodId);
            return targetMethodLiteral->GetNumArgsWithCallField();
        }
        return 0;
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
            return targetMethodLiteral->IsNoGC();
        }
        return false;
    }

    int GetMethodIndex() const
    {
        if (jsPandaFile_ == nullptr || callMethodFlagMap_ == nullptr) {
            return false;
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
            return callMethodFlagMap_->IsAotCompile(fileDesc, methodId) &&
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
            return targetMethodLiteral->GetMethodId().GetOffset();
        }
        return 0;
    }

    BuiltinsStubCSigns::ID TryGetPGOBuiltinMethodId() const;

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

    bool IsClassInstanceTypeKind() const
    {
        if (compilationEnv_->IsJitCompiler()) {
            return false;
        }
        return tsManager_->IsClassInstanceTypeKind(acc_.GetGateType(receiver_));
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

    uint32_t GetType() const
    {
        return GetFuncMethodOffsetFromPGO();
    }

    PropertyLookupResult GetPlr() const
    {
        return plr_;
    }

private:
    PropertyLookupResult GetAccessorPlr() const;
    GlobalTSTypeRef GetAccessorFuncGT() const;

    GateRef receiver_;
    CallKind kind_ {CallKind::INVALID};
    PropertyLookupResult plr_ { PropertyLookupResult() };
};

class ObjectAccessTypeInfoAccessor : public TypeInfoAccessor {
public:
    class ObjectAccessInfo final {
    public:
        explicit ObjectAccessInfo(int hclassIndex = -1, PropertyLookupResult plr = PropertyLookupResult())
            : hclassIndex_(hclassIndex), plr_(plr) {}

        void Set(int hclassIndex, PropertyLookupResult plr)
        {
            hclassIndex_ = hclassIndex;
            plr_ = plr;
        }

        int HClassIndex() const
        {
            return hclassIndex_;
        }

        PropertyLookupResult Plr() const
        {
            return plr_;
        }

    private:
        int hclassIndex_;
        PropertyLookupResult plr_;
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

    GateType GetReceiverGateType() const
    {
        if (compilationEnv_->IsJitCompiler()) {
            return acc_.GetGateType(receiver_);
        }
        return tsManager_->TryNarrowUnionType(acc_.GetGateType(receiver_));
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

    ObjectAccessInfo GetAccessInfo(size_t index) const
    {
        ASSERT(index < accessInfos_.size());
        return accessInfos_[index];
    }

protected:
    bool GeneratePlr(ProfileTyper type, ObjectAccessInfo &info, JSTaggedValue key) const;

    bool hasIllegalType_;
    ChunkVector<ObjectAccessInfo> accessInfos_;
    ChunkVector<ObjectAccessInfo> checkerInfos_;
};

class LoadObjByNameTypeInfoAccessor final : public ObjAccByNameTypeInfoAccessor {
public:
    LoadObjByNameTypeInfoAccessor(const CompilationEnv *env,
                                  Circuit *circuit,
                                  GateRef gate,
                                  Chunk *chunk);
    NO_COPY_SEMANTIC(LoadObjByNameTypeInfoAccessor);
    NO_MOVE_SEMANTIC(LoadObjByNameTypeInfoAccessor);

    size_t GetTypeCount()
    {
        return types_.size();
    }

    bool TypesIsEmpty()
    {
        return types_.size() == 0;
    }

    bool IsMono()
    {
        return types_.size() == 1;
    }

    bool IsReceiverEqHolder(size_t index)
    {
        ASSERT(index < types_.size());
        return types_[index].first == types_[index].second;
    }

private:
    void FetchPGORWTypesDual();

    bool GenerateObjectAccessInfo();

    ChunkVector<std::pair<ProfileTyper, ProfileTyper>> types_;
};

class StoreObjByNameTypeInfoAccessor final : public ObjAccByNameTypeInfoAccessor {
public:
    StoreObjByNameTypeInfoAccessor(const CompilationEnv *env,
                                   Circuit *circuit,
                                   GateRef gate,
                                   Chunk *chunk);
    NO_COPY_SEMANTIC(StoreObjByNameTypeInfoAccessor);
    NO_MOVE_SEMANTIC(StoreObjByNameTypeInfoAccessor);

    size_t GetTypeCount()
    {
        return types_.size();
    }

    bool TypesIsEmpty()
    {
        return types_.size() == 0;
    }

    bool IsMono()
    {
        return types_.size() == 1;
    }

    bool IsReceiverEqHolder(size_t index) const
    {
        return std::get<0>(types_[index]) == std::get<1>(types_[index]);
    }

    bool IsReceiverNoEqNewHolder(size_t index) const
    {
        return std::get<0>(types_[index]) != std::get<2>(types_[index]);    // 2 means 3rd object
    }

    bool IsHolderEqNewHolder(size_t index) const
    {
        return std::get<1>(types_[index]) == std::get<2>(types_[index]);    // 2 means 3rd object
    }

    GateRef GetValue() const
    {
        return value_;
    }

private:
    void FetchPGORWTypesDual();

    bool GenerateObjectAccessInfo();

    ChunkVector<std::tuple<ProfileTyper, ProfileTyper, ProfileTyper>> types_;
    GateRef value_;
};

class InstanceOfTypeInfoAccessor final : public ObjAccByNameTypeInfoAccessor {
public:
    InstanceOfTypeInfoAccessor(const CompilationEnv *env,
                                  Circuit *circuit,
                                  GateRef gate,
                                  Chunk *chunk);
    NO_COPY_SEMANTIC(InstanceOfTypeInfoAccessor);
    NO_MOVE_SEMANTIC(InstanceOfTypeInfoAccessor);

    size_t GetTypeCount()
    {
        return types_.size();
    }

    bool TypesIsEmpty()
    {
        return types_.size() == 0;
    }

    bool IsMono()
    {
        return types_.size() == 1;
    }

    JSTaggedValue GetKeyTaggedValue() const;

    GateRef GetTarget() const
    {
        return target_;
    }

private:
    void FetchPGORWTypesDual();

    bool ClassInstanceIsCallable(ProfileTyper type) const;

    bool GenerateObjectAccessInfo();

    ChunkVector<std::pair<ProfileTyper, ProfileTyper>> types_;
    GateRef target_;
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

    bool IsBuiltinInstanceType(BuiltinTypeId type) const
    {
        if (compilationEnv_->IsJitCompiler()) {
            return false;
        }
        return tsManager_->IsBuiltinInstanceType(type, GetReceiverGateType());
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
    GateRef value_;
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
                                        const CString &recordName);

    NO_COPY_SEMANTIC(CreateObjWithBufferTypeInfoAccessor);
    NO_MOVE_SEMANTIC(CreateObjWithBufferTypeInfoAccessor);

    JSTaggedValue GetHClass() const;

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
};
}   // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TYPE_INFO_ACCESSORS_H
