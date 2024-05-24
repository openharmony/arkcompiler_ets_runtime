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

#ifndef MPL2MPL_INCLUDE_SIMPLIFY_H
#define MPL2MPL_INCLUDE_SIMPLIFY_H
#include "phase_impl.h"
#include "factory.h"
#include "maple_phase_manager.h"
namespace maple {

const std::map<std::string, std::string> asmMap = {
#include "asm_map.def"
};

enum ErrorNumber {
    ERRNO_OK = EOK,
    ERRNO_INVAL = EINVAL,
    ERRNO_RANGE = ERANGE,
    ERRNO_INVAL_AND_RESET = EINVAL_AND_RESET,
    ERRNO_RANGE_AND_RESET = ERANGE_AND_RESET,
    ERRNO_OVERLAP_AND_RESET = EOVERLAP_AND_RESET
};

enum MemOpKind { MEM_OP_unknown, MEM_OP_memset, MEM_OP_memcpy, MEM_OP_memset_s, MEM_OP_memcpy_s };

// MemEntry models a memory entry with high level type information.
struct MemEntry {
    enum MemEntryKind { kMemEntryUnknown, kMemEntryPrimitive, kMemEntryStruct, kMemEntryArray };

    MemEntry() = default;
    MemEntry(BaseNode *addrExpr, MIRType *memType) : addrExpr(addrExpr), memType(memType) {}

    MemEntryKind GetKind() const
    {
        if (memType == nullptr) {
            return kMemEntryUnknown;
        }
        auto typeKind = memType->GetKind();
        if (typeKind == kTypeScalar || typeKind == kTypePointer) {
            return kMemEntryPrimitive;
        } else if (typeKind == kTypeArray) {
            return kMemEntryArray;
        } else if (memType->IsStructType()) {
            return kMemEntryStruct;
        }
        return kMemEntryUnknown;
    }

    BaseNode *BuildAsRhsExpr(MIRFunction &func) const;
    void ExpandMemsetLowLevel(int64 byte, uint64 size, MIRFunction &func, StmtNode &stmt, BlockNode &block,
                              MemOpKind memOpKind, bool debug, ErrorNumber errorNumber) const;
    static StmtNode *GenMemopRetAssign(StmtNode &stmt, MIRFunction &func, bool isLowLevel, MemOpKind memOpKind,
                                       ErrorNumber errorNumber = ERRNO_OK);

    BaseNode *addrExpr = nullptr;  // memory address
    MIRType *memType = nullptr;    // memory type, this may be nullptr for low level memory entry
};

// For simplifying memory operation, either memset or memcpy/memmove.
class SimplifyMemOp {
public:
    static MemOpKind ComputeMemOpKind(StmtNode &stmt);
    SimplifyMemOp() = default;
    virtual ~SimplifyMemOp() = default;
    explicit SimplifyMemOp(MIRFunction *func, bool debug = false) : func(func), debug(debug) {}
    void SetFunction(MIRFunction *f)
    {
        func = f;
    }
    void SetDebug(bool dbg)
    {
        debug = dbg;
    }

    bool AutoSimplify(StmtNode &stmt, BlockNode &block, bool isLowLevel);

private:
    static const uint32 thresholdMemsetExpand;
    static const uint32 thresholdMemsetSExpand;
    static const uint32 thresholdMemcpyExpand;
    static const uint32 thresholdMemcpySExpand;
    MIRFunction *func = nullptr;
    bool debug = false;
};

class Simplify : public FuncOptimizeImpl {
public:
    Simplify(MIRModule &mod, KlassHierarchy *kh, bool dump) : FuncOptimizeImpl(mod, kh, dump), mirMod(mod) {}
    Simplify(const Simplify &other) = delete;
    Simplify &operator=(const Simplify &other) = delete;
    ~Simplify() = default;
    FuncOptimizeImpl *Clone() override
    {
        CHECK_FATAL(false, "Simplify has pointer, should not be Cloned");
    }

    void ProcessStmt(StmtNode &stmt) override;
    void Finish() override;

private:
    MIRModule &mirMod;
    SimplifyMemOp simplifyMemOp;
    bool IsMathSqrt(const std::string funcName);
    bool IsMathAbs(const std::string funcName);
    bool IsMathMin(const std::string funcName);
    bool IsMathMax(const std::string funcName);
    bool IsSymbolReplaceableWithConst(const MIRSymbol &symbol) const;
    bool IsConstRepalceable(const MIRConst &mirConst) const;
    bool SimplifyMathMethod(const StmtNode &stmt, BlockNode &block);
    void SimplifyCallAssigned(StmtNode &stmt, BlockNode &block);
    BaseNode *SimplifyExpr(BaseNode &expr);
    BaseNode *ReplaceExprWithConst(DreadNode &dread);
    MIRConst *GetElementConstFromFieldId(FieldID fieldId, MIRConst *mirConst);
};

}  // namespace maple
#endif  // MPL2MPL_INCLUDE_SIMPLIFY_H
