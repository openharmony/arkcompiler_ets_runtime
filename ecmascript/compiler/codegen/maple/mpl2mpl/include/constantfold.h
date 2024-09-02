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

#ifndef MPL2MPL_INCLUDE_CONSTANTFOLD_H
#define MPL2MPL_INCLUDE_CONSTANTFOLD_H
#include "mir_nodes.h"
#include "phase_impl.h"

#include <optional>

namespace maple {
class ConstantFold : public FuncOptimizeImpl {
public:
    ConstantFold(MIRModule &mod, bool trace) : FuncOptimizeImpl(mod, trace), mirModule(&mod) {}

    explicit ConstantFold(MIRModule &mod) : FuncOptimizeImpl(mod, false), mirModule(&mod) {}

    // Fold an expression.
    // It returns a new expression if there was something to fold, or
    // nullptr otherwise.
    BaseNode *Fold(BaseNode *node);

    FuncOptimizeImpl *Clone() override
    {
        return new ConstantFold(*this);
    }

    ~ConstantFold() override
    {
        mirModule = nullptr;
    }

    template <class T>
    T CalIntValueFromFloatValue(T value, const MIRType &resultType) const;
    MIRConst *FoldFloorMIRConst(const MIRConst &cst, PrimType fromType, PrimType toType, bool isFloor = true) const;
    MIRConst *FoldRoundMIRConst(const MIRConst &cst, PrimType fromType, PrimType toType) const;
    MIRConst *FoldTypeCvtMIRConst(const MIRConst &cst, PrimType fromType, PrimType toType) const;
    MIRConst *FoldSignExtendMIRConst(Opcode opcode, PrimType resultType, uint8 size, const IntVal &val) const;
    static MIRConst *FoldIntConstBinaryMIRConst(Opcode opcode, PrimType resultType, const MIRIntConst &intConst0,
                                                const MIRIntConst &intConst1);
    MIRConst *FoldConstComparisonMIRConst(Opcode opcode, PrimType resultType, PrimType opndType,
                                          const MIRConst &const0, const MIRConst &const1) const;
    static bool IntegerOpIsOverflow(Opcode op, PrimType primType, int64 cstA, int64 cstB);
    static MIRIntConst *FoldIntConstUnaryMIRConst(Opcode opcode, PrimType resultType, const MIRIntConst *constNode);
private:
    std::pair<BaseNode*, std::optional<IntVal>> FoldBase(BaseNode *node) const;
    std::pair<BaseNode*, std::optional<IntVal>> FoldBinary(BinaryNode *node);
    std::pair<BaseNode*, std::optional<IntVal>> FoldCompare(CompareNode *node);
    std::pair<BaseNode*, std::optional<IntVal>> FoldExtractbits(ExtractbitsNode *node);
    ConstvalNode *FoldSignExtend(Opcode opcode, PrimType resultType, uint8 size, const ConstvalNode &cst) const;
    std::pair<BaseNode*, std::optional<IntVal>> FoldIread(IreadNode *node);
    std::pair<BaseNode*, std::optional<IntVal>> FoldRetype(RetypeNode *node);
    std::pair<BaseNode*, std::optional<IntVal>> FoldUnary(UnaryNode *node);
    std::pair<BaseNode*, std::optional<IntVal>> FoldTypeCvt(TypeCvtNode *node);
    ConstvalNode *FoldCeil(const ConstvalNode &cst, PrimType fromType, PrimType toType) const;
    ConstvalNode *FoldFloor(const ConstvalNode &cst, PrimType fromType, PrimType toType) const;
    ConstvalNode *FoldRound(const ConstvalNode &cst, PrimType fromType, PrimType toType) const;
    ConstvalNode *FoldTrunc(const ConstvalNode &cst, PrimType fromType, PrimType toType) const;
    ConstvalNode *FoldTypeCvt(const ConstvalNode &cst, PrimType fromType, PrimType toType) const;
    ConstvalNode *FoldConstComparison(Opcode opcode, PrimType resultType, PrimType opndType, const ConstvalNode &const0,
                                      const ConstvalNode &const1) const;
    ConstvalNode *FoldConstBinary(Opcode opcode, PrimType resultType, const ConstvalNode &const0,
                                  const ConstvalNode &const1) const;
    ConstvalNode *FoldIntConstComparison(Opcode opcode, PrimType resultType, PrimType opndType,
                                         const ConstvalNode &const0, const ConstvalNode &const1) const;
    MIRIntConst *FoldIntConstComparisonMIRConst(Opcode opcode, PrimType resultType, PrimType opndType,
                                                const MIRIntConst &intConst0, const MIRIntConst &intConst1) const;
    ConstvalNode *FoldIntConstBinary(Opcode opcode, PrimType resultType, const ConstvalNode &const0,
                                     const ConstvalNode &const1) const;
    ConstvalNode *FoldFPConstComparison(Opcode opcode, PrimType resultType, PrimType opndType,
                                        const ConstvalNode &const0, const ConstvalNode &const1) const;
    bool ConstValueEqual(int64 leftValue, int64 rightValue) const;
    bool ConstValueEqual(float leftValue, float rightValue) const;
    bool ConstValueEqual(double leftValue, double rightValue) const;
    template<typename T>
    bool FullyEqual(T leftValue, T rightValue) const;
    template<typename T>
    int64 ComparisonResult(Opcode op, T *leftConst, T *rightConst) const;
    MIRIntConst *FoldFPConstComparisonMIRConst(Opcode opcode, PrimType resultType, PrimType opndType,
                                               const MIRConst &leftConst, const MIRConst &rightConst) const;
    ConstvalNode *FoldFPConstBinary(Opcode opcode, PrimType resultType, const ConstvalNode &const0,
                                    const ConstvalNode &const1) const;
    ConstvalNode *FoldConstUnary(Opcode opcode, PrimType resultType, ConstvalNode &constNode) const;
    template <typename T>
    ConstvalNode *FoldFPConstUnary(Opcode opcode, PrimType resultType, ConstvalNode *constNode) const;
    BaseNode *NegateTree(BaseNode *node) const;
    BaseNode *Negate(BaseNode *node) const;
    BaseNode *Negate(UnaryNode *node) const;
    BaseNode *Negate(const ConstvalNode *node) const;
    BinaryNode *NewBinaryNode(BinaryNode *old, Opcode op, PrimType primType, BaseNode *lhs, BaseNode *rhs) const;
    UnaryNode *NewUnaryNode(UnaryNode *old, Opcode op, PrimType primType, BaseNode *expr) const;
    std::pair<BaseNode*, std::optional<IntVal>> DispatchFold(BaseNode *node);
    BaseNode *PairToExpr(PrimType resultType, const std::pair<BaseNode*, std::optional<IntVal>> &pair) const;
    BaseNode *SimplifyDoubleConstvalCompare(CompareNode &node, bool isRConstval, bool isGtOrLt = false) const;
    BaseNode *SimplifyDoubleCompare(CompareNode &compareNode) const;
    CompareNode *FoldConstComparisonReverse(Opcode opcode, PrimType resultType, PrimType opndType, BaseNode &l,
                                            BaseNode &r) const;
    MIRModule *mirModule;
};

}  // namespace maple
#endif  // MPL2MPL_INCLUDE_CONSTANTFOLD_H
