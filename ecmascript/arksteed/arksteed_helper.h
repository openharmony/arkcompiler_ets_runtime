/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_ARKSTEED_HELPER_H
#define ECMASCRIPT_ARKSTEED_HELPER_H

#include <memory>
#include <type_traits>
#include <utility>

#include "ecmascript/arksteed/arksteed_bb.h"
#include "ecmascript/arksteed/arksteed_framestate.h"
#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/arksteed/arksteed_graph_labeller.h"
#include "ecmascript/arksteed/arksteed_opcode.h"
#include "ecmascript/arksteed/arksteed_vertex.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::arksteed {

template <typename Signature>
class CallbackRef;

template <typename R, typename... Args>
class CallbackRef<R(Args...)> {
public:
    CallbackRef() = delete;

    template <typename Callable, typename RawCallable = std::remove_reference_t<Callable>>
    CallbackRef(Callable &&callable)
        : context_(std::addressof(callable)), trampoline_(&Trampoline<RawCallable>)
    {
        static_assert(std::is_invocable_r_v<R, RawCallable &, Args...>,
                      "CallbackRef callable does not match the required signature");
    }

    R operator()(Args... args) const
    {
        return trampoline_(context_, std::forward<Args>(args)...);
    }

private:
    template <typename Callable>
    static R Trampoline(void *context, Args... args)
    {
        return (*static_cast<Callable *>(context))(std::forward<Args>(args)...);
    }

    void *context_;
    R (*trampoline_)(void *, Args...);
};

/**
 * ArkSteedHelper - Centralized utility class for ArkSteed graph building
 *
 * Usage pattern (CRTP inheritance):
 *   class MyReducer : public ArkSteedHelper<MyReducer> {
 *   public:
 *       explicit MyReducer(Graph* graph) : ArkSteedHelper<MyReducer>(graph, this) {}
 *   };
 */
template <typename BaseT>
class ArkSteedHelper {
public:
    explicit ArkSteedHelper(Graph *graph, BaseT *base) : base_(base), graph_(graph), chunk_(graph->GetChunk()) {}

    ~ArkSteedHelper() = default;

    //==========================================================================
    //                          Vertex Creation
    //==========================================================================

    template <typename VertexT, typename... Args>
    VertexT *NewVertex(std::initializer_list<ValueVertex *> inputs, Args &&...args)
    {
        VertexT *vertex = Vertex::New<VertexT>(chunk_, inputs, std::forward<Args>(args)...);
        return FinalizeVertex(vertex);
    }

    template <typename VertexT, typename... Args>
    VertexT *NewVertex(const std::vector<ValueVertex *> &inputs, Args &&...args)
    {
        VertexT *vertex = Vertex::New<VertexT>(chunk_, inputs, std::forward<Args>(args)...);
        return FinalizeVertex(vertex);
    }

    template <typename VertexT, typename... Args>
    VertexT *NewVertexNoInput(Args &&...args)
    {
        VertexT *vertex = Vertex::New<VertexT>(chunk_, 0, std::forward<Args>(args)...);
        return FinalizeVertex(vertex);
    }

    template <typename ControlVertexT, typename... Args>
    ControlVertexT *NewControlVertex(std::initializer_list<ValueVertex *> inputs, Args &&...args)
    {
        ControlVertexT *vertex = Vertex::New<ControlVertexT>(chunk_, inputs, std::forward<Args>(args)...);
        return FinalizeControlVertex(vertex);
    }

    template <typename VertexT>
    VertexT *FinalizeVertex(VertexT *vertex)
    {
        constexpr VertexProperties props = VertexT::PROPERTIES;
        ASSERT(currentBlock_ != nullptr);
        vertex->SetOwner(currentBlock_);
        currentBlock_->AddVertex(vertex);

        // At most one of: deopt_checkpoint, eager_deopt, lazy_deopt
        static_assert(props.IsDeoptCheckpoint() + props.CanEagerDeopt() + props.CanLazyDeopt() <= 1);

        if (props.IsDeoptCheckpoint()) {
            SetupDeoptCheckpoint(vertex);
        } else if (props.CanEagerDeopt()) {
            SetupEagerDeopt(vertex);
        } else if (props.CanLazyDeopt()) {
            SetupLazyDeopt(vertex);
        }

        if (props.CanThrow()) {
            SetupExceptionHandler(vertex);
        }

        CheckSideEffect(vertex);

        // Register vertex with graph labeller if available
        RegisterVertexWithLabeller(vertex);

        return vertex;
    }

    template <typename ControlVertexT>
    ControlVertexT *FinalizeControlVertex(ControlVertexT *vertex)
    {
        constexpr VertexProperties props = ControlVertexT::PROPERTIES;
        ASSERT(currentBlock_ != nullptr);
        vertex->SetOwner(currentBlock_);
        currentBlock_->SetControlVertex(vertex);

        // Control vertices cannot have lazy deopt, throw, or write side effects
        // Note: ThrowVertex is a special case that can throw
        static_assert(!props.CanLazyDeopt() && !props.CanWrite());

        // Control vertices can have eager deopt or checkpoint
        if constexpr (props.CanEagerDeopt()) {
            SetupEagerDeopt(vertex);
        }
        if constexpr (props.IsDeoptCheckpoint()) {
            SetupDeoptCheckpoint(vertex);
        }

        // Register vertex with graph labeller if available
        RegisterVertexWithLabeller(vertex);

        return vertex;
    }

    //==========================================================================
    //                          Metadata Setup
    //==========================================================================

    template <typename VertexT>
    void RegisterVertexWithLabeller(VertexT *vertex)
    {
        // Register vertex with graph labeller if available
        ArkSteedGraphLabeller *labeller = GetCurrentGraphLabeller();
        if (labeller != nullptr) {
            labeller->RegisterVertex(vertex);
            // to do: Add provenance information (bytecode offset, source position) when available from the graph
            //        builder
        }
    }

    template <typename VertexT>
    void SetupDeoptCheckpoint(VertexT *vertex)
    {
        // to do: Attach deoptimization checkpoint info
    }

    template <typename VertexT>
    void SetupEagerDeopt(VertexT *vertex)
    {
        // to do: Attach eager deoptimization info
    }

    template <typename VertexT>
    void SetupLazyDeopt(VertexT *vertex)
    {
        // to do: Attach lazy deoptimization info
    }

    template <typename VertexT>
    void SetupExceptionHandler(VertexT *vertex)
    {
        // to do: Attach exception handler info
    }

    template <typename VertexT>
    void CheckSideEffect(VertexT *vertex)
    {
        // to do: Track possible side effects
    }

    //==========================================================================
    //                       Current Block Management
    //==========================================================================

    BB *CurrentBlock() const
    {
        return currentBlock_;
    }

    void SetCurrentBlock(BB *block)
    {
        currentBlock_ = block;
    }

    //==========================================================================
    //                   Constant Accessors (delegates to Graph)
    //==========================================================================

    ValueVertex *GetRootConstant(RootConstantVertex::RootIndex index)
    {
        return graph_->GetRootConstant(index);
    }

    ValueVertex *GetInt32Constant(int32_t value)
    {
        return graph_->GetInt32Constant(value);
    }

    ValueVertex *GetIntPtrConstant(intptr_t value)
    {
        return graph_->GetIntPtrConstant(value);
    }

    ValueVertex *GetFloat64Constant(double value)
    {
        return graph_->GetFloat64Constant(value);
    }

    ValueVertex *GetTaggedConstant(uint64_t value)
    {
        return graph_->GetTaggedConstant(value);
    }

    //==========================================================================
    //                          Graph Accessors
    //==========================================================================

    Graph *GetGraph() const
    {
        return graph_;
    }

    Chunk *GetChunk() const
    {
        return chunk_;
    }

    //==========================================================================
    //                  Base class access (for CRTP pattern)
    //==========================================================================

    BaseT *GetBase() const
    {
        return base_;
    }

protected:
    BaseT *base_;
    Graph *graph_;
    Chunk *chunk_;
    BB *currentBlock_ = nullptr;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_HELPER_H
