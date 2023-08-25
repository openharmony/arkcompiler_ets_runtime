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

#ifndef ECMASCRIPT_COMPILER_TYPE_INFERENCE_PGO_TYPE_INFER_HELPER_H
#define ECMASCRIPT_COMPILER_TYPE_INFERENCE_PGO_TYPE_INFER_HELPER_H

#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {
#define TYPE_LIST(V)                \
    V(Class, class)                 \
    V(ClassInstance, classInstance) \
    V(Builtin, builtin)             \
    V(Other, other)

struct CollectedType {
    explicit CollectedType(Chunk *chunk) :
        classTypes(chunk),
        classInstanceTypes(chunk),
        builtinTypes(chunk),
        otherTypes(chunk)
    {}

    bool AllInSameKind() const
    {
        uint8_t kind = classTypes.empty() ? 0 : 1;
        kind += classInstanceTypes.empty() ? 0 : 1;
        kind += builtinTypes.empty() ? 0 : 1;
        kind += otherTypes.empty() ? 0 : 1;
        return kind == 1;
    }

#define DEFINE_TYPE_SET(V, name)   \
    ChunkSet<GateType> name##Types;

    TYPE_LIST(DEFINE_TYPE_SET)
#undef DEFINE_TYPE_SET
};

class TypeStrategy {
public:
    TypeStrategy() = default;
    virtual ~TypeStrategy() = default;
    virtual bool CheckAndInsert(CollectedType &types, const GateType &type, TSManager *tsManager) = 0;
    virtual void Merge(ChunkSet<GateType> &inferTypes, CollectedType &types, TSManager *tsManager) = 0;
};

#define DEFINE_TYPE_STRATEGY_DERIVED_CLASS(name, ...)                                                            \
    class name##TypeStrategy : public TypeStrategy {                                                             \
    public:                                                                                                      \
        virtual bool CheckAndInsert(CollectedType &types, const GateType &type, TSManager *tsManager) override;  \
        virtual void Merge(ChunkSet<GateType> &inferTypes, CollectedType &types, TSManager *tsManager) override; \
    };

    TYPE_LIST(DEFINE_TYPE_STRATEGY_DERIVED_CLASS)
#undef DEFINE_TYPE_STRATEGY_DERIVED_CLASS

class PGOTypeInferHelper {
public:
    PGOTypeInferHelper(TSManager *tsManager) : tsManager_(tsManager)
    {
#define ADD_STRATEGY(name, ...) \
    strategies_.push_back(std::make_unique<name##TypeStrategy>());

    TYPE_LIST(ADD_STRATEGY)
#undef ADD_STRATEGY
    }
    ~PGOTypeInferHelper() = default;

    void CollectGateType(CollectedType &types, GateType tsType, PGORWOpType pgoTypes);

    ChunkSet<GateType> GetInferTypes(Chunk *chunk, CollectedType &types, JSTaggedValue prop);

private:
    inline bool NoNeedUpdate(const ChunkSet<GateType> &types) const
    {
        return types.size() <= 1;
    }

    void CheckAndInsert(CollectedType &types, GateType type);
    void UpdateType(CollectedType &types, JSTaggedValue prop);
    void InferTypeForClass(ChunkSet<GateType> &types, JSTaggedValue prop);
    void EliminateSubclassTypes(ChunkSet<GateType> &types);
    void ComputeCommonSuperClassTypes(ChunkSet<GateType> &types, JSTaggedValue prop);
    void InferTypeForBuiltin(ChunkSet<GateType> &types);

    TSManager *tsManager_ {nullptr};
    std::vector<std::unique_ptr<TypeStrategy>> strategies_ {};
};
#undef TYPE_LIST
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TYPE_INFERENCE_PGO_TYPE_INFER_HELPER_H
