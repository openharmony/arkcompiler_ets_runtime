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

#ifndef ECMASCRIPT_ARKSTEED_GRAPH_LABELLER_H
#define ECMASCRIPT_ARKSTEED_GRAPH_LABELLER_H

#include "ecmascript/arksteed/arksteed_vertex.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/log_wrapper.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::arksteed {

// Bytecode offset constants
static constexpr uint32_t kNoBytecodeOffset = UINT32_MAX;

// Forward declarations
class ArkSteedCompilationEnv;
class BB;
class Graph;

/**
 * Graph labeller for ArkSteed compiler debugging and tracing.
 *
 * This class provides unique labels for vertices in the IR graph, making it
 * easier to identify and reference vertices in debug output. It also tracks
 * provenance information (source position, bytecode offset) for each vertex.
 */
class ArkSteedGraphLabeller {
public:
    /**
     * Provenance information for a vertex - tracks where it came from.
     */
    struct Provenance {
        const JitCompilationEnv *compilationEnv = nullptr;
        uint32_t bytecodeOffset = kNoBytecodeOffset;
        // to do: Add SourcePosition when ArkSteed supports source positions
        // Missing: SourcePosition class/struct with file, line, column information
        // Required: SourcePosition type definition and SourcePosition::Unknown() factory

        Provenance() = default;
        Provenance(const JitCompilationEnv *env, uint32_t offset) : compilationEnv(env), bytecodeOffset(offset) {}

        bool IsValid() const
        {
            return compilationEnv != nullptr;
        }
    };

    /**
     * Information stored for each registered vertex.
     */
    struct VertexInfo {
        int label = -1;         // Unique label (n1, n2, ...)
        Provenance provenance;  // Where this vertex came from

        VertexInfo() = default;
        VertexInfo(int lbl, const Provenance &prov) : label(lbl), provenance(prov) {}
    };

    ArkSteedGraphLabeller() : nextVertexLabel_(1) {}
    ~ArkSteedGraphLabeller() = default;

    // ---------------------------------------------------------------------
    // Vertex registration methods
    // ---------------------------------------------------------------------

    /**
     * Register a vertex with full provenance information.
     */
    void RegisterVertex(Vertex *vertex, const JitCompilationEnv *compilationEnv, uint32_t bytecodeOffset)
    {
        if (vertices_.emplace(vertex, VertexInfo{nextVertexLabel_, {compilationEnv, bytecodeOffset}}).second) {
            nextVertexLabel_++;
        }
    }

    /**
     * Register a vertex with provenance information.
     */
    void RegisterVertex(Vertex *vertex, const Provenance *provenance)
    {
        if (provenance != nullptr) {
            RegisterVertex(vertex, provenance->compilationEnv, provenance->bytecodeOffset);
        } else {
            RegisterVertex(vertex, nullptr, kNoBytecodeOffset);
        }
    }

    /**
     * Register a vertex without provenance information.
     */
    void RegisterVertex(Vertex *vertex)
    {
        RegisterVertex(vertex, nullptr, kNoBytecodeOffset);
    }

    // ---------------------------------------------------------------------
    // Label access methods
    // ---------------------------------------------------------------------

    /**
     * Get the label for a vertex.
     * Returns -1 if the vertex is not registered.
     */
    int VertexLabel(const Vertex *vertex) const
    {
        auto it = vertices_.find(vertex);
        if (it == vertices_.end()) {
            return -1;
        }
        return it->second.label;
    }

    /**
     * Get the provenance information for a vertex.
     */
    const Provenance &GetVertexProvenance(const Vertex *vertex) const
    {
        static const Provenance kEmptyProvenance;
        auto it = vertices_.find(vertex);
        if (it == vertices_.end()) {
            return kEmptyProvenance;
        }
        return it->second.provenance;
    }

    /**
     * Get the maximum vertex label assigned so far.
     */
    int MaxVertexLabel() const
    {
        return nextVertexLabel_ - 1;
    }

    // ---------------------------------------------------------------------
    // Printing methods
    // ---------------------------------------------------------------------

    /**
     * Get vertex label as a string in the format "n<label>".
     * If hasRegallocData is true, returns "v<id>/n<label>".
     */
    std::string GetVertexLabel(const Vertex *vertex, bool hasRegallocData = false) const
    {
        if (vertex == nullptr) {
            return "<null>";
        }

        auto it = vertices_.find(vertex);
        if (it == vertices_.end()) {
            // Vertex not registered - show as unregistered
            return "<unregistered>";
        }

        std::string result;
        if (hasRegallocData && vertex->HasId()) {
            // Include vertex ID when we have register allocation data
            result = "v" + std::to_string(vertex->GetId()) + "/";
        }
        result += "n" + std::to_string(it->second.label);
        return result;
    }

    /**
     * Print a vertex label in the format "n<label>".
     * If hasRegallocData is true, also print vertex ID in format "v<id>/n<label>".
     * @deprecated Use GetVertexLabel() instead.
     */
    void PrintVertexLabel(std::ostream &os, const Vertex *vertex, bool hasRegallocData = false) const
    {
        (void)os;
        LOG_COMPILER(INFO) << GetVertexLabel(vertex, hasRegallocData);
    }

    /**
     * Get vertex input as a string with optional register allocation information.
     * @deprecated Use GetVertexInputLabel(VertexInput) instead.
     * to do: This method should be removed once all callers are updated to use VertexInput.
     */
    std::string GetVertexInputLabel(Vertex *input, bool hasRegallocData = false) const
    {
        // Create temporary VertexInput with unknown operand index (default to 0)
        // to do: Need to get actual operand index from caller context
        return GetVertexInputLabel(VertexInput(static_cast<ValueVertex *>(input), 0), hasRegallocData);
    }

    /**
     * Get vertex input as a string with optional register allocation information.
     * Includes operand index when hasRegallocData is true (format: "n<label>:<operand>").
     * to do: Operand index display currently shows placeholder value 0.
     *        Actual operand mapping requires register allocator implementation.
     */
    std::string GetVertexInputLabel(VertexInput input, bool hasRegallocData = false) const
    {
        std::string result = GetVertexLabel(input.GetVertex(), hasRegallocData);
        if (hasRegallocData) {
            // to do: This shows the operand index, but actual mapping to register allocation
            //       data requires register allocator to be implemented first.
            result += ":" + std::to_string(input.GetIndex());
        }
        return result;
    }

    /**
     * Print a vertex input with optional register allocation information.
     * @deprecated Use PrintInput(VertexInput) instead.
     * to do: This method should be removed once all callers are updated to use VertexInput.
     */
    void PrintVertexInput(std::ostream &os, Vertex *input, bool hasRegallocData = false) const
    {
        (void)os;
        // Create temporary VertexInput with unknown operand index (default to 0)
        // to do: Need to get actual operand index from caller context
        PrintInput(VertexInput(static_cast<ValueVertex *>(input), 0), hasRegallocData);
    }

    /**
     * Print a vertex input with optional register allocation information.
     * to do: When register allocator is implemented, operand index will have actual meaning.
     */
    void PrintInput(VertexInput input, bool hasRegallocData = false) const
    {
        LOG_COMPILER(INFO) << GetVertexInputLabel(input, hasRegallocData);
    }

    /**
     * Print a vertex input with optional register allocation information (ostream version).
     * to do: Consider unifying ostream and LOG_COMPILER output patterns.
     */
    void PrintInput(std::ostream &os, VertexInput input, bool hasRegallocData = false) const
    {
        (void)os;
        LOG_COMPILER(INFO) << GetVertexInputLabel(input, hasRegallocData);
    }

    /**
     * Check if a vertex is registered.
     */
    bool IsVertexRegistered(const Vertex *vertex) const
    {
        return vertices_.find(vertex) != vertices_.end();
    }

    /**
     * Get the number of registered vertices.
     */
    size_t GetRegisteredVertexCount() const
    {
        return vertices_.size();
    }

    /**
     * Clear all registered vertices (for reuse).
     */
    void Clear()
    {
        vertices_.clear();
        nextVertexLabel_ = 1;
    }

    // ---------------------------------------------------------------------
    // to do: Missing functionality
    // ---------------------------------------------------------------------
    // 1. SourcePosition support
    //    - Missing: SourcePosition class/struct with file, line, column info
    //    - Required: SourcePosition type definition and integration into Provenance
    //    - See Provenance struct for existing to do
    //
    // 2. VirtualObject special handling
    //    - Missing: VirtualObject vertex type concept in ArkSteed
    //    - Required: If VirtualObject-like vertices are added, need special
    //      handling in GetVertexLabel()
    //
    // 3. Identity vertex recursive printing
    //    - Missing: Identity vertex opcode in ArkSteed
    //    - Required: If Identity vertices are added, need recursive printing
    //      of inputs in GetVertexLabel()
    //
    // 4. Input operand information - PARTIALLY IMPLEMENTED
    //    - Base support: GetVertexInputLabel(VertexInput) added
    //    - Full functionality: Operand index display requires register allocator
    //    - Required: Register allocator implementation to provide actual operand mapping
    //    - See GetVertexInputLabel(VertexInput) method for implementation details
    //
    // 5. Enhanced input printing - PARTIALLY IMPLEMENTED
    //    - Base support: PrintInput(VertexInput) methods added
    //    - Full functionality: Actual operand index meaning depends on register allocation
    //    - Required: Update callers to pass VertexInput instead of Vertex*
    //    - Required: Register allocator to map operands to physical registers
    // ---------------------------------------------------------------------

private:
    // Map from vertex pointer to its label and provenance
    std::map<const Vertex *, VertexInfo> vertices_;

    // Next label to assign
    int nextVertexLabel_;

    NO_COPY_SEMANTIC(ArkSteedGraphLabeller);
    NO_MOVE_SEMANTIC(ArkSteedGraphLabeller);
};

/**
 * Scope class for managing graph labeller lifecycle.
 * Sets thread-local labeller on construction, clears it on destruction.
 */
class ArkSteedGraphLabellerScope {
public:
    explicit ArkSteedGraphLabellerScope(ArkSteedGraphLabeller *labeller);
    ~ArkSteedGraphLabellerScope();

private:
    ArkSteedGraphLabeller *labeller_;
};

// ---------------------------------------------------------------------
// Thread-local labeller management
// ---------------------------------------------------------------------

// Thread-local pointer to the current graph labeller
extern thread_local ArkSteedGraphLabeller *g_threadGraphLabeller;

/**
 * Get the current graph labeller for this thread.
 * Returns nullptr if no labeller is active.
 */
inline ArkSteedGraphLabeller *GetCurrentGraphLabeller()
{
    return g_threadGraphLabeller;
}

/**
 * Check if a graph labeller is currently active.
 */
inline bool HasCurrentGraphLabeller()
{
    return g_threadGraphLabeller != nullptr;
}

// ---------------------------------------------------------------------
// Debug printing helpers (conditionally compiled)
// ---------------------------------------------------------------------

#ifdef ARKSTEED_ENABLE_GRAPH_PRINTER

/**
 * Helper class for printing vertices with labels.
 * Usage: os << PrintVertex(vertex);
 */
class PrintVertex {
public:
    explicit PrintVertex(const Vertex *vertex, bool hasRegallocData = false)
        : vertex_(vertex), hasRegallocData_(hasRegallocData)
    {}

    void Print(std::ostream &os) const;

private:
    std::string BuildOutputString() const;

    const Vertex *vertex_;
    bool hasRegallocData_;
};

/**
 * Helper class for printing vertex labels only.
 * Usage: os << PrintVertexLabel(vertex);
 */
class PrintVertexLabel {
public:
    explicit PrintVertexLabel(const Vertex *vertex) : vertex_(vertex) {}

    void Print(std::ostream &os) const;

private:
    const Vertex *vertex_;
};

/**
 * Helper class for printing brief vertex information.
 * Usage: os << PrintVertexBrief(vertex);
 */
class PrintVertexBrief {
public:
    explicit PrintVertexBrief(const Vertex *vertex) : vertex_(vertex) {}

    void Print(std::ostream &os) const
    {
        (void)os;
        if (vertex_ == nullptr) {
            LOG_COMPILER(INFO) << "<null>";
            return;
        }

        ArkSteedGraphLabeller *labeller = GetCurrentGraphLabeller();
        if (labeller == nullptr) {
            LOG_COMPILER(INFO) << "<no labeller> (" << OpcodeToString(vertex_->GetOpcode()) << ")";
        } else {
            LOG_COMPILER(INFO) << labeller->GetVertexLabel(vertex_, false) << " ("
                               << OpcodeToString(vertex_->GetOpcode()) << ")";
        }
    }

private:
    const Vertex *vertex_;
};

#else  // ARKSTEED_ENABLE_GRAPH_PRINTER not defined

// Dummy implementations when graph printing is disabled
class PrintVertex {
public:
    explicit PrintVertex(const Vertex *vertex, bool hasRegallocData = false) {}
    void Print(std::ostream &os) const
    {
        (void)os;
        LOG_COMPILER(INFO) << "<graph printing disabled>";
    }
};

class PrintVertexLabel {
public:
    explicit PrintVertexLabel(const Vertex *vertex) {}
    void Print(std::ostream &os) const
    {
        (void)os;
        LOG_COMPILER(INFO) << "<graph printing disabled>";
    }
};

class PrintVertexBrief {
public:
    explicit PrintVertexBrief(const Vertex *vertex) {}
    void Print(std::ostream &os) const
    {
        (void)os;
        LOG_COMPILER(INFO) << "<graph printing disabled>";
    }
};

#endif  // ARKSTEED_ENABLE_GRAPH_PRINTER

// ---------------------------------------------------------------------
// Stream operators for printing helpers
// ---------------------------------------------------------------------

inline std::ostream &operator<<(std::ostream &os, const PrintVertex &printer)
{
    printer.Print(os);
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const PrintVertexLabel &printer)
{
    printer.Print(os);
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const PrintVertexBrief &printer)
{
    printer.Print(os);
    return os;
}

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_GRAPH_LABELLER_H