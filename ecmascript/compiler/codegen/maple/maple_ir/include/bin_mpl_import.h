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

#ifndef MAPLE_IR_INCLUDE_BIN_MPL_IMPORT_H
#define MAPLE_IR_INCLUDE_BIN_MPL_IMPORT_H
#include "mir_module.h"
#include "mir_nodes.h"
#include "mir_preg.h"
#include "parser_opt.h"
#include "mir_builder.h"
#include "ea_connection_graph.h"
namespace maple {
class BinaryMplImport {
public:
    explicit BinaryMplImport(MIRModule &md) : mod(md), mirBuilder(&md) {}
    BinaryMplImport &operator=(const BinaryMplImport &) = delete;
    BinaryMplImport(const BinaryMplImport &) = delete;

    virtual ~BinaryMplImport()
    {
        for (MIRStructType *structPtr : tmpStruct) {
            delete structPtr;
        }
        for (MIRClassType *classPtr : tmpClass) {
            delete classPtr;
        }
        for (MIRInterfaceType *interfacePtr : tmpInterface) {
            delete interfacePtr;
        }
    }

    uint64 GetBufI() const
    {
        return bufI;
    }
    void SetBufI(uint64 bufIVal)
    {
        bufI = bufIVal;
    }

    bool IsBufEmpty() const
    {
        return buf.empty();
    }
    size_t GetBufSize() const
    {
        return buf.size();
    }

    int32 GetContent(int64 key) const
    {
        return content.at(key);
    }

    void SetImported(bool importedVal)
    {
        imported = importedVal;
    }

    bool Import(const std::string &modid, bool readSymbols = false, bool readSe = false);
    bool ImportForSrcLang(const std::string &modid, MIRSrcLang &srcLang);
    MIRSymbol *GetOrCreateSymbol(TyIdx tyIdx, GStrIdx strIdx, MIRSymKind mclass, MIRStorageClass sclass,
                                 MIRFunction *func, uint8 scpID);
    int32 ReadInt();
    int64 ReadNum();

private:
    void ReadContentField();
    void ReadStrField();
    void ReadHeaderField();
    void ReadTypeField();
    void ReadSymField();
    void ReadSymTabField();
    void ReadCgField();
    EAConnectionGraph *ReadEaCgField();
    void ReadEaField();
    EACGBaseNode &InEaCgNode(EAConnectionGraph &newEaCg);
    void InEaCgBaseNode(EACGBaseNode &base, EAConnectionGraph &newEaCg, bool firstPart);
    void InEaCgActNode(EACGActualNode &actual);
    void InEaCgFieldNode(EACGFieldNode &field, EAConnectionGraph &newEaCg);
    void InEaCgObjNode(EACGObjectNode &obj, EAConnectionGraph &newEaCg);
    void InEaCgRefNode(EACGRefNode &ref);
    CallInfo *ImportCallInfo();
    void MergeDuplicated(PUIdx methodPuidx, std::vector<CallInfo *> &targetSet, std::vector<CallInfo *> &newSet);
    void ReadSeField();
    void Jump2NextField();
    void Reset();
    void SkipTotalSize();
    void ImportFieldsOfStructType(FieldVector &fields, uint32 methodSize);
    MIRType &InsertInTypeTables(MIRType &ptype);
    void InsertInHashTable(MIRType &ptype);
    void UpdateMethodSymbols();
    void ImportConstBase(MIRConstKind &kind, MIRTypePtr &type);
    MIRConst *ImportConst(MIRFunction *func);
    GStrIdx ImportStr();
    UStrIdx ImportUsrStr();
    MIRType *CreateMirType(MIRTypeKind kind, GStrIdx strIdx, int64 tag) const;
    MIRGenericInstantType *CreateMirGenericInstantType(GStrIdx strIdx) const;
    MIRBitFieldType *CreateBitFieldType(uint8 fieldsize, PrimType pt, GStrIdx strIdx) const;
    void CompleteAggInfo(TyIdx tyIdx);
    TyIdx ImportJType(bool forPointedType = false);
    TyIdx ImportType();
    void ImportTypeBase(PrimType &primType, GStrIdx &strIdx, bool &nameIsLocal);
    void InSymTypeTable();
    void ImportTypePairs(std::vector<TypePair> &insVecType);
    TypeAttrs ImportTypeAttrs();
    MIRPragmaElement *ImportPragmaElement();
    MIRPragma *ImportPragma();
    void ImportFieldPair(FieldPair &fp);
    void ImportMethodPair(MethodPair &memPool);
    void ImportMethodsOfStructType(MethodVector &methods);
    void ImportStructTypeData(MIRStructType &type);
    void ImportInterfacesOfClassType(std::vector<TyIdx> &interfaces);
    void ImportInfoIsStringOfStructType(MIRStructType &type);
    void ImportInfoOfStructType(MIRStructType &type);
    void ImportPragmaOfStructType(MIRStructType &type);
    void SetClassTyidxOfMethods(MIRStructType &type);
    void ImportClassTypeData(MIRClassType &type);
    void ImportInterfaceTypeData(MIRInterfaceType &type);
    PUIdx ImportFunction();
    MIRSymbol *InSymbol(MIRFunction *func);
    void ImportInfoVector(MIRInfoVector &infoVector, MapleVector<bool> &infoVectorIsString);
    void ImportLocalTypeNameTable(MIRTypeNameTable *typeNameTab);
    void ImportFuncIdInfo(MIRFunction *func);
    MIRSymbol *ImportLocalSymbol(MIRFunction *func);
    PregIdx ImportPreg(MIRFunction *func);
    LabelIdx ImportLabel(MIRFunction *func);
    void ImportFormalsStIdx(MIRFunction *func);
    void ImportAliasMap(MIRFunction *func);
    void ImportSrcPos(SrcPosition &pos);
    void ImportBaseNode(Opcode &o, PrimType &typ);
    PUIdx ImportFuncViaSym(MIRFunction *func);
    BaseNode *ImportExpression(MIRFunction *func);
    void ImportReturnValues(MIRFunction *func, CallReturnVector *retv);
    BlockNode *ImportBlockNode(MIRFunction *fn);
    void ReadFunctionBodyField();
    void ReadFileAt(const std::string &modid, int32 offset);
    uint8 Read();
    int64 ReadInt64();
    void ReadAsciiStr(std::string &str);
    int32 GetIPAFileIndex(std::string &name);

    bool inCG = false;
    bool inIPA = false;
    bool imported = true;            // used only by irbuild to convert to ascii
    bool importingFromMplt = false;  // decided based on magic number
    uint64 bufI = 0;
    std::vector<uint8> buf;
    std::map<int64, int32> content;
    MIRModule &mod;
    MIRBuilder mirBuilder;
    std::vector<GStrIdx> gStrTab;
    std::vector<UStrIdx> uStrTab;
    std::vector<MIRStructType *> tmpStruct;
    std::vector<MIRClassType *> tmpClass;
    std::vector<MIRInterfaceType *> tmpInterface;
    std::vector<TyIdx> typTab;
    std::vector<MIRFunction *> funcTab;
    std::vector<MIRSymbol *> symTab;
    std::vector<MIRSymbol *> localSymTab;
    std::vector<PregIdx> localPregTab;
    std::vector<LabelIdx> localLabelTab;
    std::vector<CallInfo *> callInfoTab;
    std::vector<EACGBaseNode *> eaCgTab;
    std::vector<MIRSymbol *> methodSymbols;
    std::vector<bool> definedLabels;
    std::string importFileName;
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_BIN_MPL_IMPORT_H
