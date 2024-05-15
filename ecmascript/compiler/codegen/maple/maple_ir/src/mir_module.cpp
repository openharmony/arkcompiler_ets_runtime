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

#include "mir_module.h"
#include "mir_const.h"
#include "mir_preg.h"
#include "mir_function.h"
#include "mir_builder.h"
#include "debug_info.h"
#include "intrinsics.h"
#include "bin_mplt.h"

namespace maple {
#if MIR_FEATURE_FULL  // to avoid compilation error when MIR_FEATURE_FULL=0
MIRModule::MIRModule(const std::string &fn)
    : memPool(new ThreadShareMemPool(memPoolCtrler, "maple_ir mempool")),
      pragmaMemPool(memPoolCtrler.NewMemPool("pragma mempool", false /* isLcalPool */)),
      memPoolAllocator(memPool),
      pragmaMemPoolAllocator(pragmaMemPool),
      functionList(memPoolAllocator.Adapter()),
      importedMplt(memPoolAllocator.Adapter()),
      typeDefOrder(memPoolAllocator.Adapter()),
      externStructTypeSet(std::less<TyIdx>(), memPoolAllocator.Adapter()),
      symbolSet(std::less<StIdx>(), memPoolAllocator.Adapter()),
      symbolDefOrder(memPoolAllocator.Adapter()),
      out(LogInfo::MapleLogger()),
      fileName(fn),
      fileInfo(memPoolAllocator.Adapter()),
      fileInfoIsString(memPoolAllocator.Adapter()),
      fileData(memPoolAllocator.Adapter()),
      srcFileInfo(memPoolAllocator.Adapter()),
      importFiles(memPoolAllocator.Adapter()),
      importPaths(memPoolAllocator.Adapter()),
      asmDecls(memPoolAllocator.Adapter()),
      classList(memPoolAllocator.Adapter()),
      optimizedFuncs(memPoolAllocator.Adapter()),
      optimizedFuncsType(memPoolAllocator.Adapter()),
      puIdxFieldInitializedMap(std::less<PUIdx>(), memPoolAllocator.Adapter()),
      inliningGlobals(memPoolAllocator.Adapter()),
      partO2FuncList(memPoolAllocator.Adapter()),
      safetyWarningMap(memPoolAllocator.Adapter())
{
    GlobalTables::GetGsymTable().SetModule(this);
    typeNameTab = memPool->New<MIRTypeNameTable>(memPoolAllocator);
    mirBuilder = memPool->New<MIRBuilder>(this);
    dbgInfo = memPool->New<DebugInfo>(this);
    IntrinDesc::InitMIRModule(this);
}

MIRModule::~MIRModule()
{
    for (MIRFunction *mirFunc : functionList) {
        mirFunc->ReleaseCodeMemory();
    }
    ReleasePragmaMemPool();
    delete memPool;
    delete binMplt;
}

MemPool *MIRModule::CurFuncCodeMemPool() const
{
    if (useFuncCodeMemPoolTmp) {
        return CurFunction()->GetCodeMemPoolTmp();
    }
    return CurFunction()->GetCodeMemPool();
}

MapleAllocator *MIRModule::CurFuncCodeMemPoolAllocator() const
{
    MIRFunction *curFunc = CurFunction();
    CHECK_FATAL(curFunc != nullptr, "curFunction is null");
    return &curFunc->GetCodeMempoolAllocator();
}

MapleAllocator &MIRModule::GetCurFuncCodeMPAllocator() const
{
    MIRFunction *curFunc = CurFunction();
    CHECK_FATAL(curFunc != nullptr, "curFunction is null");
    return curFunc->GetCodeMPAllocator();
}

void MIRModule::AddExternStructType(TyIdx tyIdx)
{
    (void)externStructTypeSet.insert(tyIdx);
}

void MIRModule::AddExternStructType(const MIRType *t)
{
    DEBUG_ASSERT(t != nullptr, "MIRType is null");
    (void)externStructTypeSet.insert(t->GetTypeIndex());
}

void MIRModule::AddSymbol(StIdx stIdx)
{
    auto it = symbolSet.find(stIdx);
    if (it == symbolSet.end()) {
        symbolDefOrder.push_back(stIdx);
    }
    (void)symbolSet.insert(stIdx);
}

void MIRModule::AddSymbol(const MIRSymbol *s)
{
    DEBUG_ASSERT(s != nullptr, "s is null");
    AddSymbol(s->GetStIdx());
}

void MIRModule::DumpGlobals(bool emitStructureType) const
{
    if (flavor != kFlavorUnknown) {
        LogInfo::MapleLogger() << "flavor " << flavor << '\n';
    }
    if (srcLang != kSrcLangUnknown) {
        LogInfo::MapleLogger() << "srclang " << srcLang << '\n';
    }
    LogInfo::MapleLogger() << "id " << id << '\n';
    if (globalMemSize != 0) {
        LogInfo::MapleLogger() << "globalmemsize " << globalMemSize << '\n';
    }
    if (globalBlkMap != nullptr) {
        LogInfo::MapleLogger() << "globalmemmap = [ ";
        auto *p = reinterpret_cast<uint32 *>(globalBlkMap);
        LogInfo::MapleLogger() << std::hex;
        while (p < reinterpret_cast<uint32 *>(globalBlkMap + globalMemSize)) {
            LogInfo::MapleLogger() << std::hex << "0x" << *p << " ";
            p++;
        }
        LogInfo::MapleLogger() << std::dec << "]\n";
    }
    if (globalWordsTypeTagged != nullptr) {
        LogInfo::MapleLogger() << "globalwordstypetagged = [ ";
        auto *p = reinterpret_cast<uint32 *>(globalWordsTypeTagged);
        LogInfo::MapleLogger() << std::hex;
        while (p < reinterpret_cast<uint32 *>(globalWordsTypeTagged + BlockSize2BitVectorSize(globalMemSize))) {
            LogInfo::MapleLogger() << std::hex << "0x" << *p << " ";
            ++p;
        }
        LogInfo::MapleLogger() << std::dec << "]\n";
    }
    if (globalWordsRefCounted != nullptr) {
        LogInfo::MapleLogger() << "globalwordsrefcounted = [ ";
        auto *p = reinterpret_cast<uint32 *>(globalWordsRefCounted);
        LogInfo::MapleLogger() << std::hex;
        while (p < reinterpret_cast<uint32 *>(globalWordsRefCounted + BlockSize2BitVectorSize(globalMemSize))) {
            LogInfo::MapleLogger() << std::hex << "0x" << *p << " ";
            ++p;
        }
        LogInfo::MapleLogger() << std::dec << "]\n";
    }
    LogInfo::MapleLogger() << "numfuncs " << numFuncs << '\n';
    if (!importFiles.empty()) {
        // Output current module's mplt on top, imported ones at below
        for (auto it = importFiles.rbegin(); it != importFiles.rend(); ++it) {
            LogInfo::MapleLogger() << "import \"" << GlobalTables::GetStrTable().GetStringFromStrIdx(*it) << "\"\n";
        }
    }
    if (!importPaths.empty()) {
        size_t size = importPaths.size();
        for (size_t i = 0; i < size; ++i) {
            LogInfo::MapleLogger() << "importpath \"" << GlobalTables::GetStrTable().GetStringFromStrIdx(importPaths[i])
                                   << "\"\n";
        }
    }
    if (!asmDecls.empty()) {
        size_t size = asmDecls.size();
        for (size_t i = 0; i < size; ++i) {
            LogInfo::MapleLogger() << "asmdecl ";
            EmitStr(asmDecls[i]);
        }
    }
    if (entryFuncName.length()) {
        LogInfo::MapleLogger() << "entryfunc &" << entryFuncName << '\n';
    }
    if (!fileInfo.empty()) {
        LogInfo::MapleLogger() << "fileinfo {\n";
        size_t size = fileInfo.size();
        for (size_t i = 0; i < size; ++i) {
            LogInfo::MapleLogger() << "  @" << GlobalTables::GetStrTable().GetStringFromStrIdx(fileInfo[i].first)
                                   << " ";
            if (!fileInfoIsString[i]) {
                LogInfo::MapleLogger() << "0x" << std::hex << fileInfo[i].second;
            } else {
                LogInfo::MapleLogger() << "\""
                                       << GlobalTables::GetStrTable().GetStringFromStrIdx(GStrIdx(fileInfo[i].second))
                                       << "\"";
            }
            if (i < size - 1) {
                LogInfo::MapleLogger() << ",\n";
            } else {
                LogInfo::MapleLogger() << "}\n";
            }
        }
        LogInfo::MapleLogger() << std::dec;
    }
    if (!srcFileInfo.empty()) {
        LogInfo::MapleLogger() << "srcfileinfo {\n";
        size_t size = srcFileInfo.size();
        size_t i = 0;
        for (auto infoElem : srcFileInfo) {
            LogInfo::MapleLogger() << "  " << infoElem.second;
            LogInfo::MapleLogger() << " \"" << GlobalTables::GetStrTable().GetStringFromStrIdx(infoElem.first) << "\"";
            if (i++ < size - 1) {
                LogInfo::MapleLogger() << ",\n";
            } else {
                LogInfo::MapleLogger() << "}\n";
            }
        }
    }
    if (!fileData.empty()) {
        LogInfo::MapleLogger() << "filedata {\n";
        size_t size = fileData.size();
        for (size_t i = 0; i < size; ++i) {
            LogInfo::MapleLogger() << "  @" << GlobalTables::GetStrTable().GetStringFromStrIdx(fileData[i].first)
                                   << " ";
            size_t dataSize = fileData[i].second.size();
            for (size_t j = 0; j < dataSize; ++j) {
                uint8 data = fileData[i].second[j];
                LogInfo::MapleLogger() << "0x" << std::hex << static_cast<uint32>(data);
                if (j < dataSize - 1) {
                    LogInfo::MapleLogger() << ' ';
                }
            }
            if (i < size - 1) {
                LogInfo::MapleLogger() << ",\n";
            } else {
                LogInfo::MapleLogger() << "}\n";
            }
        }
        LogInfo::MapleLogger() << std::dec;
    }
    if (flavor < kMmpl || flavor == kFlavorLmbc) {
        for (auto it = typeDefOrder.begin(); it != typeDefOrder.end(); ++it) {
            TyIdx tyIdx = typeNameTab->GetTyIdxFromGStrIdx(*it);
            const std::string &name = GlobalTables::GetStrTable().GetStringFromStrIdx(*it);
            MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
            DEBUG_ASSERT(type != nullptr, "type should not be nullptr here");
            bool isStructType = type->IsStructType();
            if (isStructType) {
                auto *structType = static_cast<MIRStructType *>(type);
                // still emit what in extern_structtype_set_
                if (!emitStructureType &&
                    externStructTypeSet.find(structType->GetTypeIndex()) == externStructTypeSet.end()) {
                    continue;
                }
                if (structType->IsImported()) {
                    continue;
                }
            }

            LogInfo::MapleLogger() << "type $" << name << " ";
            if (type->GetKind() == kTypeByName) {
                LogInfo::MapleLogger() << "void";
            } else if (type->GetNameStrIdx() == *it) {
                type->Dump(1, true);
            } else {
                type->Dump(1);
            }
            LogInfo::MapleLogger() << '\n';
        }
        if (someSymbolNeedForwDecl) {
            // an extra pass thru the global symbol table to print forward decl
            for (auto sit = symbolSet.begin(); sit != symbolSet.end(); ++sit) {
                MIRSymbol *s = GlobalTables::GetGsymTable().GetSymbolFromStidx((*sit).Idx());
                if (s->IsNeedForwDecl()) {
                    s->Dump(false, 0, true);
                }
            }
        }
        for (auto sit = symbolDefOrder.begin(); sit != symbolDefOrder.end(); ++sit) {
            MIRSymbol *s = GlobalTables::GetGsymTable().GetSymbolFromStidx((*sit).Idx());
            CHECK_FATAL(s != nullptr, "nullptr check");
            if (!s->IsDeleted() && !s->GetIsImported() && !s->GetIsImportedDecl()) {
                s->Dump(false, 0);
            }
        }
    }
}

void MIRModule::Dump(bool emitStructureType, const std::unordered_set<std::string> *dumpFuncSet) const
{
    DumpGlobals(emitStructureType);
    DumpFunctionList(dumpFuncSet);
}

void MIRModule::DumpGlobalArraySymbol() const
{
    for (StIdx stIdx : symbolSet) {
        MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
        DEBUG_ASSERT(symbol != nullptr, "null ptr check");
        MIRType *symbolType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(symbol->GetTyIdx());
        DEBUG_ASSERT(symbolType != nullptr, "null ptr check");
        if (symbolType == nullptr || symbolType->GetKind() != kTypeArray) {
            continue;
        }
        symbol->Dump(false, 0);
    }
}

void MIRModule::Emit(const std::string &outFileName) const
{
    std::ofstream file;
    // Change cout's buffer to file.
    std::streambuf *backup = LogInfo::MapleLogger().rdbuf();
    LogInfo::MapleLogger().rdbuf(file.rdbuf());
    file.open(outFileName, std::ios::trunc);
    DumpGlobals();
    for (MIRFunction *mirFunc : functionList) {
        mirFunc->Dump();
    }
    // Restore cout's buffer.
    LogInfo::MapleLogger().rdbuf(backup);
    file.close();
}

void MIRModule::DumpFunctionList(const std::unordered_set<std::string> *dumpFuncSet) const
{
    for (MIRFunction *mirFunc : functionList) {
        if (dumpFuncSet == nullptr || dumpFuncSet->empty()) {
            mirFunc->Dump();
        } else {  // dump only if this func matches any name in *dumpFuncSet
            const std::string &name = mirFunc->GetName();
            bool matched = false;
            for (std::string elem : *dumpFuncSet) {
                if (name.find(elem.c_str()) != std::string::npos) {
                    matched = true;
                    break;
                }
            }
            if (matched) {
                mirFunc->Dump();
            }
        }
    }
}

void MIRModule::OutputFunctionListAsciiMpl(const std::string &phaseName)
{
    std::string fileStem;
    std::string::size_type lastDot = fileName.find_last_of('.');
    if (lastDot == std::string::npos) {
        fileStem = fileName.append(phaseName);
    } else {
        fileStem = fileName.substr(0, lastDot).append(phaseName);
    }
    std::string outfileName;
    if (flavor >= kMmpl) {
        outfileName = fileStem.append(".mmpl");
    } else {
        outfileName = fileStem.append(".mpl");
    }
    std::ofstream mplFile;
    mplFile.open(outfileName, std::ios::app);
    std::streambuf *backup = LogInfo::MapleLogger().rdbuf();
    LogInfo::MapleLogger().rdbuf(mplFile.rdbuf());  // change cout's buffer to that of file
    DumpGlobalArraySymbol();
    DumpFunctionList(nullptr);
    LogInfo::MapleLogger().rdbuf(backup);  // restore cout's buffer
    mplFile.close();
}

void MIRModule::DumpToFile(const std::string &fileNameStr, bool emitStructureType) const
{
    std::ofstream file;
    file.open(fileNameStr, std::ios::trunc);
    if (!file.is_open()) {
        ERR(kLncErr, "Cannot open %s", fileNameStr.c_str());
        return;
    }
    // Change cout's buffer to file.
    std::streambuf *backup = LogInfo::MapleLogger().rdbuf();
    LogInfo::MapleLogger().rdbuf(file.rdbuf());
    Dump(emitStructureType);
    // Restore cout's buffer.
    LogInfo::MapleLogger().rdbuf(backup);
    file.close();
}

void MIRModule::DumpDefType()
{
    for (auto it = typeDefOrder.begin(); it != typeDefOrder.end(); ++it) {
        TyIdx tyIdx = typeNameTab->GetTyIdxFromGStrIdx(*it);
        const std::string &name = GlobalTables::GetStrTable().GetStringFromStrIdx(*it);
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
        DEBUG_ASSERT(type != nullptr, "type should not be nullptr here");
        bool isStructType = type->IsStructType();
        if (isStructType) {
            auto *structType = static_cast<MIRStructType *>(type);
            if (structType->IsImported()) {
                continue;
            }
        }
        LogInfo::MapleLogger() << "type $" << name << " ";
        if (type->GetKind() == kTypeByName) {
            LogInfo::MapleLogger() << "void";
        } else if (type->GetNameStrIdx() == *it) {
            type->Dump(1, true);
        } else {
            type->Dump(1);
        }
        LogInfo::MapleLogger() << '\n';
    }
}

void MIRModule::DumpInlineCandidateToFile(const std::string &fileNameStr)
{
    if (optimizedFuncs.empty()) {
        return;
    }
    std::ofstream file;
    // Change cout's buffer to file.
    std::streambuf *backup = LogInfo::MapleLogger().rdbuf();
    LogInfo::MapleLogger().rdbuf(file.rdbuf());
    file.open(fileNameStr, std::ios::trunc);
    if (IsCModule()) {
        DumpDefType();
    }
    // dump global variables needed for inlining file
    for (auto symbolIdx : inliningGlobals) {
        MIRSymbol *s = GlobalTables::GetGsymTable().GetSymbolFromStidx(symbolIdx);
        DEBUG_ASSERT(s != nullptr, "null ptr check");
        if (s->GetStorageClass() == kScFstatic) {
            if (s->IsNeedForwDecl()) {
                // const string, including initialization
                s->Dump(false, 0, false);
            }
        }
    }
    for (auto symbolIdx : inliningGlobals) {
        MIRSymbol *s = GlobalTables::GetGsymTable().GetSymbolFromStidx(symbolIdx);
        DEBUG_ASSERT(s != nullptr, "null ptr check");
        MIRStorageClass sc = s->GetStorageClass();
        if (s->GetStorageClass() == kScFstatic) {
            if (!s->IsNeedForwDecl()) {
                // const string, including initialization
                s->Dump(false, 0, false);
            }
        } else if (s->GetSKind() == kStFunc) {
            s->GetFunction()->Dump(true);
        } else {
            // static fields as extern
            s->SetStorageClass(kScExtern);
            s->Dump(false, 0, true);
        }
        s->SetStorageClass(sc);
    }
    for (auto *func : optimizedFuncs) {
        func->SetWithLocInfo(false);
        func->Dump();
    }
    // Restore cout's buffer.
    LogInfo::MapleLogger().rdbuf(backup);
    file.close();
}

// This is not efficient. Only used in debug mode for now.
const std::string &MIRModule::GetFileNameFromFileNum(uint32 fileNum) const
{
    GStrIdx nameIdx(0);
    for (auto &info : srcFileInfo) {
        if (info.second == fileNum) {
            nameIdx = info.first;
            break;
        }
    }
    return GlobalTables::GetStrTable().GetStringFromStrIdx(nameIdx);
}

void MIRModule::DumpToHeaderFile(bool binaryMplt, const std::string &outputName)
{
    std::string outfileName;
    std::string fileNameLocal = !outputName.empty() ? outputName : fileName;
    std::string::size_type lastDot = fileNameLocal.find_last_of('.');
    if (lastDot == std::string::npos) {
        outfileName = fileNameLocal.append(".mplt");
    } else {
        outfileName = fileNameLocal.substr(0, lastDot).append(".mplt");
    }
    if (binaryMplt) {
        BinaryMplt binaryMpltTmp(*this);
        binaryMpltTmp.Export(outfileName);
    } else {
        std::ofstream mpltFile;
        mpltFile.open(outfileName, std::ios::trunc);
        std::streambuf *backup = LogInfo::MapleLogger().rdbuf();
        LogInfo::MapleLogger().rdbuf(mpltFile.rdbuf());  // change cout's buffer to that of file
        for (std::pair<std::u16string, MIRSymbol *> entity : GlobalTables::GetConstPool().GetConstU16StringPool()) {
            LogInfo::MapleLogger() << "var $";
            entity.second->DumpAsLiteralVar();
            LogInfo::MapleLogger() << '\n';
        }
        for (auto it = classList.begin(); it != classList.end(); ++it) {
            TyIdx curTyIdx(*it);
            MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(curTyIdx);
            const std::string &name = GlobalTables::GetStrTable().GetStringFromStrIdx(type->GetNameStrIdx());
            if (type->GetKind() == kTypeClass || type->GetKind() == kTypeInterface) {
                auto *structType = static_cast<MIRStructType *>(type);
                // skip imported class/interface and incomplete types
                if (!structType->IsImported() && !structType->IsIncomplete()) {
                    LogInfo::MapleLogger() << "type $" << name << " ";
                    type->Dump(1, true);
                    LogInfo::MapleLogger() << '\n';
                }
            }
        }
        /* restore cout */
        LogInfo::MapleLogger().rdbuf(backup);
        mpltFile.close();
    }
}

void MIRModule::DumpTypeTreeToCxxHeaderFile(MIRType &ty, std::unordered_set<MIRType *> &dumpedClasses) const
{
    if (dumpedClasses.find(&ty) != dumpedClasses.end()) {
        return;
    }
    // first, insert ty to the dumped_classes to prevent infinite recursion
    (void)dumpedClasses.insert(&ty);
    DEBUG_ASSERT(ty.GetKind() == kTypeClass || ty.GetKind() == kTypeStruct || ty.GetKind() == kTypeUnion ||
                     ty.GetKind() == kTypeInterface,
                 "Unexpected MIRType.");
    /* No need to emit interfaces; because "interface variables are
       final and static by default and methods are public and abstract"
     */
    if (ty.GetKind() == kTypeInterface) {
        return;
    }
    // dump all of its parents
    if (srcLang == kSrcLangC || srcLang == kSrcLangCPlusPlus) {
        DEBUG_ASSERT((ty.GetKind() == kTypeStruct || ty.GetKind() == kTypeUnion),
                     "type should be either struct or union");
    } else {
        DEBUG_ASSERT(false, "source languages other than C/C++ are not supported yet");
    }
    if (srcLang == kSrcLangC || srcLang == kSrcLangCPlusPlus) {
        // how to access parent fields????
        DEBUG_ASSERT(false, "not yet implemented");
    }
}

void MIRModule::DumpToCxxHeaderFile(std::set<std::string> &leafClasses, const std::string &pathToOutf) const
{
    std::ofstream mpltFile;
    mpltFile.open(pathToOutf, std::ios::trunc);
    std::streambuf *backup = LogInfo::MapleLogger().rdbuf();
    LogInfo::MapleLogger().rdbuf(mpltFile.rdbuf());  // change cout's buffer to that of file
    char *headerGuard = strdup(pathToOutf.c_str());
    CHECK_FATAL(headerGuard != nullptr, "strdup failed");
    for (char *p = headerGuard; *p; ++p) {
        if (!isalnum(*p)) {
            *p = '_';
        } else if (isalpha(*p) && islower(*p)) {
            *p = toupper(*p);
        }
    }
    // define a hash table
    std::unordered_set<MIRType *> dumpedClasses;
    const char *prefix = "__SRCLANG_UNKNOWN_";
    if (srcLang == kSrcLangC || srcLang == kSrcLangCPlusPlus) {
        prefix = "__SRCLANG_CXX_";
    }
    LogInfo::MapleLogger() << "#ifndef " << prefix << headerGuard << "__\n";
    LogInfo::MapleLogger() << "#define " << prefix << headerGuard << "__\n";
    LogInfo::MapleLogger() << "/* this file is compiler-generated; do not edit */\n\n";
    LogInfo::MapleLogger() << "#include <stdint.h>\n";
    LogInfo::MapleLogger() << "#include <complex.h>\n";
    for (auto &s : leafClasses) {
        CHECK_FATAL(!s.empty(), "string is null");
        GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(s);
        TyIdx tyIdx = typeNameTab->GetTyIdxFromGStrIdx(strIdx);
        MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
        if (ty == nullptr) {
            continue;
        }
        DEBUG_ASSERT(ty->GetKind() == kTypeClass || ty->GetKind() == kTypeStruct || ty->GetKind() == kTypeUnion ||
                         ty->GetKind() == kTypeInterface,
                     "");
        DumpTypeTreeToCxxHeaderFile(*ty, dumpedClasses);
    }
    LogInfo::MapleLogger() << "#endif /* " << prefix << headerGuard << "__ */\n";
    /* restore cout */
    LogInfo::MapleLogger().rdbuf(backup);
    free(headerGuard);
    headerGuard = nullptr;
    mpltFile.close();
}

void MIRModule::DumpClassToFile(const std::string &path) const
{
    std::string strPath(path);
    strPath.append("/");
    for (auto it : typeNameTab->GetGStrIdxToTyIdxMap()) {
        const std::string &name = GlobalTables::GetStrTable().GetStringFromStrIdx(it.first);
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(it.second);
        std::string outClassFile(name);
        /* replace class name / with - */
        std::replace(outClassFile.begin(), outClassFile.end(), '/', '-');
        (void)outClassFile.insert(0, strPath);
        outClassFile.append(".mpl");
        std::ofstream mplFile;
        mplFile.open(outClassFile, std::ios::trunc);
        std::streambuf *backup = LogInfo::MapleLogger().rdbuf();
        LogInfo::MapleLogger().rdbuf(mplFile.rdbuf());
        /* dump class type */
        LogInfo::MapleLogger() << "type $" << name << " ";
        if (type->GetNameStrIdx() == it.first && type->GetKind() != kTypeByName) {
            type->Dump(1, true);
        } else {
            type->Dump(1);
        }
        LogInfo::MapleLogger() << '\n';
        /* restore cout */
        LogInfo::MapleLogger().rdbuf(backup);
        mplFile.close();
        ;
    }
}

MIRFunction *MIRModule::FindEntryFunction()
{
    for (MIRFunction *currFunc : functionList) {
        if (currFunc->GetName() == entryFuncName) {
            entryFunc = currFunc;
            return currFunc;
        }
    }
    return nullptr;
}

// given the phase name (including '.' at beginning), output the program in the
// module to the file with given file suffix, and file stem from
// this->fileName appended with phaseName
void MIRModule::OutputAsciiMpl(const char *phaseName, const char *suffix,
                               const std::unordered_set<std::string> *dumpFuncSet, bool emitStructureType,
                               bool binaryform)
{
    DEBUG_ASSERT(!(emitStructureType && binaryform), "Cannot emit type info in .bpl");
    std::string fileStem;
    std::string::size_type lastDot = fileName.find_last_of('.');
    if (lastDot == std::string::npos) {
        fileStem = fileName.append(phaseName);
    } else {
        fileStem = fileName.substr(0, lastDot).append(phaseName);
    }
    std::string outfileName;
    outfileName = fileStem + suffix;
    if (!binaryform) {
        std::ofstream mplFile;
        mplFile.open(outfileName, std::ios::trunc);
        std::streambuf *backup = LogInfo::MapleLogger().rdbuf();
        LogInfo::MapleLogger().rdbuf(mplFile.rdbuf());  // change LogInfo::MapleLogger()'s buffer to that of file
        Dump(emitStructureType, dumpFuncSet);
        LogInfo::MapleLogger().rdbuf(backup);  // restore LogInfo::MapleLogger()'s buffer
        mplFile.close();
    } else {
        BinaryMplt binaryMplt(*this);
        binaryMplt.GetBinExport().not2mplt = true;
        binaryMplt.Export(outfileName);
    }
    std::ofstream mplFile;
    mplFile.open(outfileName, std::ios::trunc);
    std::streambuf *backup = LogInfo::MapleLogger().rdbuf();
    LogInfo::MapleLogger().rdbuf(mplFile.rdbuf());  // change cout's buffer to that of file
    Dump(emitStructureType);
    if (withDbgInfo) {
        dbgInfo->Dump(0);
    }
    LogInfo::MapleLogger().rdbuf(backup);  // restore cout's buffer
    mplFile.close();
}

uint32 MIRModule::GetFileinfo(GStrIdx strIdx) const
{
    for (auto &infoElem : fileInfo) {
        if (infoElem.first == strIdx) {
            return infoElem.second;
        }
    }
    DEBUG_ASSERT(false, "should not be here");
    return 0;
}

std::string MIRModule::GetFileNameAsPostfix() const
{
    std::string fileNameStr = namemangler::kFileNameSplitterStr;
    if (!fileInfo.empty()) {
        // option 1: file name in INFO
        uint32 fileNameIdx = GetFileinfo(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("INFO_filename"));
        fileNameStr += GlobalTables::GetStrTable().GetStringFromStrIdx(GStrIdx(fileNameIdx));
    } else {
        // option 2: src file name removing ext name.
        if (GetSrcFileInfo().size() != 0) {
            GStrIdx idx = GetSrcFileInfo()[0].first;
            const std::string kStr = GlobalTables::GetStrTable().GetStringFromStrIdx(idx);
            DEBUG_ASSERT(kStr.find_last_of('.') != kStr.npos, "not found .");
            fileNameStr += kStr.substr(0, kStr.find_last_of('.'));
        } else {
            DEBUG_ASSERT(0, "No fileinfo and no srcfileinfo in mpl file");
        }
    }
    for (char &c : fileNameStr) {
        if (!isalpha(c) && !isdigit(c) && c != '_' && c != '$') {
            c = '_';
        }
    }
    return fileNameStr;
}

void MIRModule::AddClass(TyIdx tyIdx)
{
    (void)classList.insert(tyIdx);
}

void MIRModule::RemoveClass(TyIdx tyIdx)
{
    (void)classList.erase(tyIdx);
}

#endif  // MIR_FEATURE_FULL
void MIRModule::ReleaseCurFuncMemPoolTmp()
{
    CurFunction()->ReleaseMemory();
}

void MIRModule::SetFuncInfoPrinted() const
{
    CurFunction()->SetInfoPrinted();
}

void MIRModule::InitPartO2List(const std::string &list)
{
    if (list.empty()) {
        return;
    }
    SetHasPartO2List(true);
    std::ifstream infile(list);
    if (!infile.is_open()) {
        LogInfo::MapleLogger(kLlErr) << "Cannot open partO2 function list file " << list << '\n';
        return;
    }
    std::string str;

    while (getline(infile, str)) {
        if (str.empty()) {
            continue;
        }
        GStrIdx funcStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(str);
        partO2FuncList.insert(funcStrIdx);
    }
    infile.close();
}

bool MIRModule::HasNotWarned(uint32 position, uint32 stmtOriginalID)
{
    auto warnedOp = safetyWarningMap.find(position);
    if (warnedOp == safetyWarningMap.end()) {
        MapleSet<uint32> opSet(memPoolAllocator.Adapter());
        opSet.emplace(stmtOriginalID);
        safetyWarningMap.emplace(std::pair<uint32, MapleSet<uint32>>(position, std::move(opSet)));
        return true;
    }
    if (warnedOp->second.find(stmtOriginalID) == warnedOp->second.end()) {
        warnedOp->second.emplace(stmtOriginalID);
        return true;
    }
    return false;
}
}  // namespace maple
