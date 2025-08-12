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

// This define the lowest level MAPLE IR data structures that are compatible
// with both the C++ and C coding environments of MAPLE
#ifndef MAPLE_INCLUDE_VM_CMPL_V2
#define MAPLE_INCLUDE_VM_CMPL_V2
// Still need constant value from MIR
#include <cstdint>
#include "mir_config.h"
#include "types_def.h"
#include "opcodes.h"
#include "prim_types.h"
#include "intrinsics.h"
#include "mir_module.h"

namespace maple {
constexpr uint32 kTwoBitVectors = 2;
struct MirFuncT {  // 28B
    uint16 frameSize;
    uint16 upFormalSize;
    uint16 moduleID;
    uint32 funcSize;               // size of code in words
    uint8 *formalWordsTypetagged;  // bit vector where the Nth bit tells whether
    // the Nth word in the formal parameters area
    // addressed upward from %%FP (that means
    // the word at location (%%FP + N*4)) has
    // typetag; if yes, the typetag is the word
    // at (%%FP + N*4 + 4); the bitvector's size
    // is given by BlockSize2BitvectorSize(upFormalSize)
    uint8 *localWordsTypetagged;  // bit vector where the Nth bit tells whether
    // the Nth word in the local stack frame
    // addressed downward from %%FP (that means
    // the word at location (%%FP - N*4)) has
    // typetag; if yes, the typetag is the word
    // at (%%FP - N*4 + 4); the bitvector's size
    // is given by BlockSize2BitvectorSize(frameSize)
    uint8 *formalWordsRefCounted;  // bit vector where the Nth bit tells whether
    // the Nth word in the formal parameters area
    // addressed upward from %%FP (that means
    // the word at location (%%FP + N*4)) points to
    // a dynamic memory block that needs reference
    // count; the bitvector's size is given by
    // BlockSize2BitvectorSize(upFormalSize)
    uint8 *localWordsRefCounted;  // bit vector where the Nth bit tells whether
    // the Nth word in the local stack frame
    // addressed downward from %%FP (that means
    // the word at location (%%FP - N*4)) points to
    // a dynamic memory block that needs reference
    // count; the bitvector's size is given by
    // BlockSize2BitvectorSize(frameSize)
    // uint16 numlabels // removed. label table size
    // StmtNode **lbl2stmt // lbl2stmt table, removed
    // the first statement immediately follow MirFuncT
    // since it starts with expression, BaseNodeT* is returned
    void *FirstInst() const
    {
        return reinterpret_cast<uint8 *>(const_cast<MirFuncT *>(this)) + sizeof(MirFuncT);
    }

    // there are 4 bitvectors that follow the function code
    uint32 FuncCodeSize() const
    {
        return funcSize - (kTwoBitVectors * BlockSize2BitVectorSize(upFormalSize)) -
               (kTwoBitVectors * BlockSize2BitVectorSize(frameSize));
    }
};

struct MirModuleT {
public:
    MIRFlavor flavor;    // should be kCmpl
    MIRSrcLang srcLang;  // the source language
    uint16 id;
    uint32 globalMemSize;  // size of storage space for all global variables
    uint8 *globalBlkMap;   // the memory map of the block containing all the
    // globals, for specifying static initializations
    uint8 *globalWordsTypetagged;  // bit vector where the Nth bit tells whether
    // the Nth word in globalBlkMap has typetag;
    // if yes, the typetag is the N+1th word; the
    // bitvector's size is given by
    // BlockSize2BitvectorSize(globalMemSize)
    uint8 *globalWordsRefCounted;  // bit vector where the Nth bit tells whether
    // the Nth word points to a reference-counted
    // dynamic memory block; the bitvector's size
    // is given by BlockSize2BitvectorSize(globalMemSize)
    PUIdx mainFuncID;          // the entry function; 0 if no main function
    uint32 numFuncs;           // because puIdx 0 is reserved, numFuncs is also the highest puIdx
    MirFuncT **funcs;          // list of all funcs in the module.
#if 1                          // the js2mpl buld always set HAVE_MMAP to 1 // binmir file mmap info
    int binMirImageFd;         // file handle for mmap
#endif                         // HAVE_MMAP
    void *binMirImageStart;    // binimage memory start
    uint32 binMirImageLength;  // binimage memory size
    MirFuncT *FuncFromPuIdx(PUIdx puIdx) const
    {
        MIR_ASSERT(puIdx <= numFuncs);  // puIdx starts from 1
        return funcs[puIdx - 1];
    }

    MirModuleT() = default;
    ~MirModuleT() = default;
    MirFuncT *MainFunc() const
    {
        return (mainFuncID == 0) ? static_cast<MirFuncT *>(nullptr) : FuncFromPuIdx(mainFuncID);
    }

    void SetCurFunction(MirFuncT *f)
    {
        curFunction = f;
    }

    MirFuncT *GetCurFunction() const
    {
        return curFunction;
    }

    MIRSrcLang GetSrcLang() const
    {
        return srcLang;
    }

private:
    MirFuncT *curFunction = nullptr;
};

// At this stage, MirConstT don't need all information in MIRConst
// Note: only be used within Constval node:
// Warning: it's different from full feature MIR.
// only support 32bit int const (lower 32bit). higher 32bit are tags
union MirIntConstT {
    int64 value;
    uint32 val[2];  // ARM target load/store 2 32bit val instead of 1 64bit
};

// currently in VM, only intconst are used.
using MirConstT = MirIntConstT;

// It's a stacking of POD data structure to allow precise memory layout
// control and emulate the inheritance relationship of corresponding C++
// data structures to keep the interface consistent (as much as possible).
//
// Rule:
// 1. base struct should be the first member (to allow safe pointer casting)
// 2. each node (just ops, no data) should be of either 4B or 8B.
// 3. casting the node to proper base type to access base type's fields.
//
// Current memory layout of nodes follows the postfix notation:
// Each operand instruction is positioned immediately before its parent or
// next operand. Memory layout of sub-expressions tree is done recursively.
// E.g. the code for (a + b) contains 3 instructions, starting with the READ a,
// READ b, and then followed by ADD.
// For (a + (b - c)), it is:
//
// READ a
// READ b
// READ c
// SUB
// ADD
//
// BaseNodeT is an abstraction of expression.
struct BaseNodeT {  // 4B
    Opcode op;
    PrimType ptyp;
    uint8 typeFlag;  // a flag to speed up type related operations in the VM
    uint8 numOpnds;  // only used for N-ary operators, switch and rangegoto
    // operands immediately before each node
    virtual size_t NumOpnds() const
    {
        if (op == OP_switch || op == OP_rangegoto) {
            return 1;
        }
        return numOpnds;
    }

    virtual uint8 GetNumOpnds() const
    {
        return numOpnds;
    }
    virtual void SetNumOpnds(uint8 num)
    {
        numOpnds = num;
    }

    virtual Opcode GetOpCode() const
    {
        return op;
    }

    virtual void SetOpCode(Opcode o)
    {
        op = o;
    }

    virtual PrimType GetPrimType() const
    {
        return ptyp;
    }

    virtual void SetPrimType(PrimType type)
    {
        ptyp = type;
    }

    BaseNodeT() : op(OP_undef), ptyp(kPtyInvalid), typeFlag(0), numOpnds(0) {}

    virtual ~BaseNodeT() = default;
};

// typeFlag is a 8bit flag to provide short-cut information for its
// associated PrimType, because many type related information extraction
// is not very lightweight.
// Here is the convention:
// |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
//   dyn    f     i     sc    c     (log2(size))
//
// bit 0 - bit 3 is for type size information. (now not used in VM?)
//   bit 0-2 represents the size of concrete types (not void/aggregate)
//   it's the result of log2 operation on the real size to fit in 3 bits.
//   which has the following correspondence:
//   | 2 | 1 | 0 |              type size (in Bytes)
//     0   0   0                      1
//     0   0   1                      2
//     0   1   0                      4
//     0   1   1                      8
//     1   0   0                     16
//
// bit 3 is the flag of "concrete types", i.e., types we know the type
// details.
// when it's 1, the bit0-2 size are valid
// when it's 0, the size of the type is 0, and bit0-2 are meaningless.
//
// bit 4 is for scalar types (1 if it's a scalar type)
// bit 5 is for integer types (1 if it's an integer type)
// bit 6 is for floating types (1 if it's a floating type)
// bit 7 is for dynamic types (1 if it's a dynamic type)
//
// refer to mirtypes.h/mirtypes.cpp in maple_ir directory for more information.
const int32 kTypeflagZero = 0x00;
const int32 kTypeflagDynMask = 0x80;
const int32 kTypeflagFloatMask = 0x40;
const int32 kTypeflagIntergerMask = 0x20;
const int32 kTypeflagScalarMask = 0x10;
const int32 kTypeflagConcreteMask = 0x08;
const int32 kTypeflagSizeMask = 0x07;
const int32 kTypeflagDynFloatMask = (kTypeflagDynMask | kTypeflagFloatMask);
const int32 kTypeflagDynIntergerMask = (kTypeflagDynMask | kTypeflagIntergerMask);
inline bool IsDynType(uint8 typeFlag)
{
    return ((typeFlag & kTypeflagDynMask) != kTypeflagZero);
}

inline bool IsDynFloat(uint8 typeFlag)
{
    return ((typeFlag & kTypeflagDynFloatMask) == kTypeflagDynFloatMask);
}

inline bool IsDynInteger(uint8 typeFlag)
{
    return ((typeFlag & kTypeflagDynIntergerMask) == kTypeflagDynIntergerMask);
}

// IsFloat means "is statically floating types", i.e., float, but not dynamic
inline bool IsFloat(uint8 typeFlag)
{
    return ((typeFlag & kTypeflagDynFloatMask) == kTypeflagFloatMask);
}

inline bool IsScalarType(uint8 typeFlag)
{
    return ((typeFlag & kTypeflagScalarMask) != kTypeflagZero);
}

inline Opcode GetOpcode(const BaseNodeT &nodePtr)
{
    return nodePtr.op;
}

inline PrimType GetPrimType(const BaseNodeT &nodePtr)
{
    return nodePtr.ptyp;
}

inline uint32 GetOperandsNum(const BaseNodeT &nodePtr)
{
    return nodePtr.numOpnds;
}

using UnaryNodeT = BaseNodeT;             // alias
struct TypecvtNodeT : public BaseNodeT {  // 8B
    PrimType fromPTyp;
    uint8 fromTypeFlag;  // a flag to speed up type related operations
    uint8 padding[2];
    PrimType FromType() const
    {
        return fromPTyp;
    }
};

struct ExtractbitsNodeT : public BaseNodeT {  // 8B
    uint8 bOffset;
    uint8 bSize;
    uint16 padding;
};

using BinaryNodeT = BaseNodeT;
// Add expression types to compare node, to
// facilitate the evaluation of postorder stored kCmpl
// Note: the two operands should have the same type if they're
//       not dynamic types
struct CompareNodeT : public BaseNodeT {  // 8B
    PrimType opndType;                    // type of operands.
    uint8 opndTypeFlag;                   // typeFlag of opntype.
    uint8 padding[2];                     // every compare node has two opnds.
};

using NaryNodeT = BaseNodeT;
// need to guarantee MIRIntrinsicID is 4B
// Note: this is not supported by c++0x
struct IntrinsicopNodeT : public BaseNodeT {  // 8B
    MIRIntrinsicID intrinsic;
};

struct ConstvalNodeT : public BaseNodeT {  // 4B + 8B const value
    MirConstT *Constval() const
    {
        auto *tempPtr = const_cast<ConstvalNodeT *>(this);
        return (reinterpret_cast<MirConstT *>(reinterpret_cast<uint8 *>(tempPtr) + sizeof(ConstvalNodeT)));
    }
};

// full MIR exported a pointer to MirConstT
inline MirConstT *GetConstval(const ConstvalNodeT &node)
{
    return node.Constval();
}

// SizeoftypeNode shouldn't be seen here
struct AddrofNodeT : public BaseNodeT {  // 12B
    StIdx stIdx;
    FieldID fieldID;
};

using DreadNodeT = AddrofNodeT;              // same shape.

struct RegreadNodeT : public BaseNodeT {  // 8B
    PregIdx regIdx;                       // 32bit, negative if special register
};

}  // namespace maple
#endif  // MAPLE_INCLUDE_VM_CMPL_V2
