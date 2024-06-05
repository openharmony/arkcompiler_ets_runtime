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

#include "class_hierarchy.h"
#include <iostream>
#include <fstream>
#include "option.h"

// Class Hierarchy Analysis
// This phase is a foundation phase of compilation. This phase build
// the class hierarchy for both this module and all modules it depends
// on. So many phases rely on this phase's analysis result, such as
// call graph, ssa and so on.
// The main procedure shows as following.
// A. Based on the information read from mplts, it collects all class that
//    declared in modules. And creates a Klass for each class.
// B. Fill class method info. Connect superclass<->subclass and
//    interface->implementation edges.
// C. Tag All Throwable class and its child class.
// D. In the case of "class C implements B; interface B extends A;",
//    we need to add a link between C and A. So we recursively traverse
//    Klass and collect all interfaces it implements.
// E. Topological Sort
// F. Based on Topological Sort Order, for each virtual method in a class,
//    we collect all its potential implementation. If the number of
//    potential implementations is 1, it means all virtual calls to this
//    method can be easily devirtualized.
namespace maple {
bool KlassHierarchy::traceFlag = false;

bool IsSystemPreloadedClass(const std::string &className)
{
    return false;
}

Klass::Klass(MIRStructType *type, MapleAllocator *alc)
    : structType(type),
      alloc(alc),
      superKlasses(alloc->Adapter()),
      subKlasses(alloc->Adapter()),
      implKlasses(alloc->Adapter()),
      implInterfaces(alloc->Adapter()),
      methods(alloc->Adapter()),
      strIdx2Method(alloc->Adapter()),
      strIdx2CandidateMap(alloc->Adapter())
{
    DEBUG_ASSERT(type != nullptr, "type is nullptr in Klass::Klass!");
    DEBUG_ASSERT(type->GetKind() == kTypeClass || type->GetKind() == kTypeInterface || type->IsIncomplete(),
                 "runtime check error");
}

void Klass::DumpKlassMethods() const
{
    if (methods.empty()) {
        return;
    }
    LogInfo::MapleLogger() << "   class member methods:\n";
    for (MIRFunction *method : methods) {
        LogInfo::MapleLogger() << "   \t" << method->GetName() << " , ";
        method->GetFuncSymbol()->GetAttrs().DumpAttributes();
        LogInfo::MapleLogger() << "\n";
    }
    for (const auto &m2cPair : strIdx2CandidateMap) {
        LogInfo::MapleLogger() << "   \t" << GlobalTables::GetStrTable().GetStringFromStrIdx(m2cPair.first)
                               << "   , # of target:" << m2cPair.second->size() << "\n";
    }
}

void Klass::DumpKlassImplKlasses() const
{
    if (implKlasses.empty()) {
        return;
    }
    LogInfo::MapleLogger() << "  implemented by:\n";
    for (Klass *implKlass : implKlasses) {
        LogInfo::MapleLogger() << "   \t@implbyclass_idx " << implKlass->structType->GetTypeIndex() << "\n";
    }
}

void Klass::DumpKlassImplInterfaces() const
{
    if (implInterfaces.empty()) {
        return;
    }
    LogInfo::MapleLogger() << "  implements:\n";
    for (Klass *interface : implInterfaces) {
        LogInfo::MapleLogger() << "   \t@implinterface_idx " << interface->structType->GetTypeIndex() << "\n";
    }
}

void Klass::DumpKlassSuperKlasses() const
{
    if (superKlasses.empty()) {
        return;
    }
    LogInfo::MapleLogger() << "   superclasses:\n";
    for (Klass *superKlass : superKlasses) {
        LogInfo::MapleLogger() << "   \t@superclass_idx " << superKlass->structType->GetTypeIndex() << "\n";
    }
}

void Klass::DumpKlassSubKlasses() const
{
    if (subKlasses.empty()) {
        return;
    }
    LogInfo::MapleLogger() << "   subclasses:\n";
    for (Klass *subKlass : subKlasses) {
        LogInfo::MapleLogger() << "   \t@subclass_idx " << subKlass->structType->GetTypeIndex() << "\n";
    }
}

void Klass::Dump() const
{
    // Dump detailed class info
    LogInfo::MapleLogger() << "class \" " << GetKlassName() << " \" @class_id " << structType->GetTypeIndex() << "\n";
    DumpKlassSuperKlasses();
    DumpKlassSubKlasses();
    DumpKlassImplInterfaces();
    DumpKlassImplKlasses();
    DumpKlassMethods();
}

MIRFunction *Klass::GetClosestMethod(GStrIdx funcName) const
{
    MapleVector<MIRFunction *> *cands = GetCandidates(funcName);
    if (cands != nullptr && !cands->empty()) {
        return cands->at(0);
    }
    return nullptr;
}

void Klass::DelMethod(const MIRFunction &func)
{
    if (GetMethod(func.GetBaseFuncNameWithTypeStrIdx()) == &func) {
        strIdx2Method.erase(func.GetBaseFuncNameWithTypeStrIdx());
    }
    for (auto it = methods.begin(); it != methods.end(); ++it) {
        if (*it == &func) {
            methods.erase(it--);
            return;
        }
    }
}

// This for class only, which only has 0 or 1 super class
Klass *Klass::GetSuperKlass() const
{
    switch (superKlasses.size()) {
        case 0:
            return nullptr;
        case 1:
            return *superKlasses.begin();
        default:
            LogInfo::MapleLogger() << GetKlassName() << "\n";
            CHECK_FATAL(false, "GetSuperKlass expects less than one super class");
    }
}

bool Klass::IsKlassMethod(const MIRFunction *func) const
{
    if (func == nullptr) {
        return false;
    }
    for (MIRFunction *method : methods) {
        if (method == func) {
            return true;
        }
    }
    return false;
}

bool Klass::ImplementsKlass() const
{
    if (IsInterface() || IsInterfaceIncomplete()) {
        return false;
    }
    MIRClassType *classType = GetMIRClassType();
    DEBUG_ASSERT(classType != nullptr, "null ptr check");
    return (!classType->GetInterfaceImplemented().empty());
}

MapleVector<MIRFunction *> *Klass::GetCandidates(GStrIdx mnameNoklassStrIdx) const
{
    auto it = strIdx2CandidateMap.find(mnameNoklassStrIdx);
    return ((it != strIdx2CandidateMap.end()) ? (it->second) : nullptr);
}

MIRFunction *Klass::GetUniqueMethod(GStrIdx mnameNoklassStrIdx) const
{
    if (structType->IsIncomplete()) {
        return nullptr;
    }
    auto it = strIdx2CandidateMap.find(mnameNoklassStrIdx);
    if (it != strIdx2CandidateMap.end()) {
        MapleVector<MIRFunction *> *fv = it->second;
        if (fv != nullptr && fv->size() == 1) {
            return fv->at(0);
        }
    }
    return nullptr;
}

bool Klass::IsVirtualMethod(const MIRFunction &func) const
{
    // May add other checking conditions in future
    return (func.GetAttr(FUNCATTR_virtual) && !func.GetAttr(FUNCATTR_private) && !func.GetAttr(FUNCATTR_abstract));
}

void Klass::CountVirtMethBottomUp()
{
    MapleVector<MIRFunction *> *vec;
    GStrIdx strIdx;
    for (Klass *subKlass : subKlasses) {
        CHECK_FATAL(subKlass != nullptr, "nullptr check failed");
        for (const auto &pair : subKlass->strIdx2CandidateMap) {
            strIdx = pair.first;
            if (strIdx2CandidateMap.find(strIdx) == strIdx2CandidateMap.end()) {
                continue;
            }
            vec = strIdx2CandidateMap[strIdx];
            MapleVector<MIRFunction *> *subv = pair.second;
            if (!vec->empty() && !subv->empty() && vec->at(0) == subv->at(0)) {
                // If this class and subclass share the same default implementation,
                // then we have to avoid duplicated counting.
                vec->insert(vec->end(), subv->begin() + 1, subv->end());
            } else {
                vec->insert(vec->end(), subv->begin(), subv->end());
            }
        }
    }
}

const MIRFunction *Klass::HasMethod(const std::string &funcname) const
{
    for (auto *method : methods) {
        if (method->GetBaseFuncNameWithType().find(funcname) != std::string::npos) {
            return method;
        }
    }
    return nullptr;
}

Klass *KlassHierarchy::GetKlassFromStrIdx(GStrIdx strIdx) const
{
    auto it = strIdx2KlassMap.find(strIdx);
    return ((it != strIdx2KlassMap.end()) ? (it->second) : nullptr);
}

Klass *KlassHierarchy::GetKlassFromTyIdx(TyIdx tyIdx) const
{
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    return (type != nullptr ? GetKlassFromStrIdx(type->GetNameStrIdx()) : nullptr);
}

Klass *KlassHierarchy::GetKlassFromFunc(const MIRFunction *func) const
{
    return (func != nullptr ? GetKlassFromStrIdx(func->GetBaseClassNameStrIdx()) : nullptr);
}

Klass *KlassHierarchy::GetKlassFromName(const std::string &name) const
{
    return GetKlassFromStrIdx(GlobalTables::GetStrTable().GetStrIdxFromName(name));
}

Klass *KlassHierarchy::GetKlassFromLiteral(const std::string &name) const
{
    return GetKlassFromStrIdx(GlobalTables::GetStrTable().GetStrIdxFromName(name));
}

// check if super is a superclass of base
// 1/0/-1: true/false/unknown
int KlassHierarchy::IsSuperKlass(TyIdx superTyIdx, TyIdx baseTyIdx) const
{
    if (superTyIdx == 0u || baseTyIdx == 0u) {
        return -1;
    }
    if (superTyIdx == baseTyIdx) {
        return 1;
    }
    Klass *super = GetKlassFromTyIdx(superTyIdx);
    Klass *base = GetKlassFromTyIdx(baseTyIdx);
    if (super == nullptr || base == nullptr) {
        return -1;
    }
    while (base != nullptr) {
        if (base == super) {
            return 1;
        }
        base = base->GetSuperKlass();
    }
    return 0;
}

bool KlassHierarchy::IsSuperKlass(const Klass *super, const Klass *base) const
{
    if (super == nullptr || base == nullptr || base->IsInterface()) {
        return false;
    }
    while (base != nullptr) {
        if (base == super) {
            return true;
        }
        base = base->GetSuperKlass();
    }
    return false;
}

// Interface
bool KlassHierarchy::IsSuperKlassForInterface(const Klass *super, const Klass *base) const
{
    if (super == nullptr || base == nullptr) {
        return false;
    }
    if (!super->IsInterface() || !base->IsInterface()) {
        return false;
    }
    std::vector<const Klass *> tmpVector;
    tmpVector.push_back(base);
    for (size_t idx = 0; idx < tmpVector.size(); ++idx) {
        if (tmpVector[idx] == super) {
            return true;
        }
        for (const Klass *superKlass : tmpVector[idx]->GetSuperKlasses()) {
            tmpVector.push_back(superKlass);
        }
    }
    return false;
}

bool KlassHierarchy::IsInterfaceImplemented(Klass *interface, const Klass *base) const
{
    if (interface == nullptr || base == nullptr) {
        return false;
    }
    if (!interface->IsInterface() || !base->IsClass()) {
        return false;
    }
    // All the implemented interfaces and their parent interfaces
    // are directly stored in this set, so no need to look up super
    return (base->GetImplInterfaces().find(interface) != base->GetImplInterfaces().end());
}

int KlassHierarchy::GetFieldIDOffsetBetweenClasses(const Klass &super, const Klass &base) const
{
    int offset = 0;
    const Klass *superPtr = &super;
    const Klass *basePtr = &base;
    while (basePtr != superPtr) {
        basePtr = basePtr->GetSuperKlass();
        CHECK_FATAL(basePtr != nullptr, "null ptr check");
        offset++;
    }
    return offset;
}

bool KlassHierarchy::UpdateFieldID(TyIdx baseTypeIdx, TyIdx targetTypeIdx, FieldID &fldID) const
{
    MIRType *baseType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(baseTypeIdx);
    MIRType *targetType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(targetTypeIdx);
    if (baseType->GetKind() == kTypePointer && targetType->GetKind() == kTypePointer) {
        baseType = static_cast<const MIRPtrType *>(baseType)->GetPointedType();
        targetType = static_cast<const MIRPtrType *>(targetType)->GetPointedType();
    }
    if (baseType->GetKind() != kTypeClass || targetType->GetKind() != kTypeClass) {
        return false;
    }
    Klass *baseKlass = GetKlassFromTyIdx(baseType->GetTypeIndex());
    DEBUG_ASSERT(baseKlass != nullptr, "null ptr check");
    Klass *targetKlass = GetKlassFromTyIdx(targetType->GetTypeIndex());
    DEBUG_ASSERT(targetKlass != nullptr, "null ptr check");
    if (IsSuperKlass(baseKlass, targetKlass)) {
        fldID += GetFieldIDOffsetBetweenClasses(*baseKlass, *targetKlass);
        return true;
    } else if (IsSuperKlass(targetKlass, baseKlass)) {
        fldID -= GetFieldIDOffsetBetweenClasses(*targetKlass, *baseKlass);
        return true;
    }
    return false;
}

bool KlassHierarchy::NeedClinitCheckRecursively(const Klass &kl) const
{
    if (kl.HasFlag(kClassRuntimeVerify)) {
        return true;
    }
    const Klass *klass = &kl;
    if (klass->IsClass()) {
        while (klass != nullptr) {
            if (klass->GetClinit()) {
                return true;
            }
            klass = klass->GetSuperKlass();
        }
        for (Klass *implInterface : kl.GetImplInterfaces()) {
            if (implInterface->GetClinit()) {
                for (auto &func : implInterface->GetMethods()) {
                    if (!func->GetAttr(FUNCATTR_abstract) && !func->GetAttr(FUNCATTR_static)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }
    if (klass->IsInterface()) {
        return klass->GetClinit();
    }
    return true;
}

// Get lowest common ancestor for two classes
Klass *KlassHierarchy::GetLCA(Klass *klass1, Klass *klass2) const
{
    std::vector<Klass *> v1, v2;
    while (klass1 != nullptr) {
        v1.push_back(klass1);
        klass1 = klass1->GetSuperKlass();
    }
    while (klass2 != nullptr) {
        v2.push_back(klass2);
        klass2 = klass2->GetSuperKlass();
    }
    Klass *result = nullptr;
    size_t size1 = v1.size();
    size_t size2 = v2.size();
    size_t min = (size1 < size2) ? size1 : size2;
    for (size_t i = 1; i <= min; ++i) {
        CHECK_FATAL(size1 > 0, "must not be zero");
        if (v1[size1 - i] != v2[size2 - i]) {
            break;
        }
        result = v1[size1 - i];
    }
    return result;
}

TyIdx KlassHierarchy::GetLCA(TyIdx ty1, TyIdx ty2) const
{
    Klass *result = GetLCA(GetKlassFromTyIdx(ty1), GetKlassFromTyIdx(ty2));
    return (result != nullptr ? result->GetTypeIdx() : TyIdx(0));
}

GStrIdx KlassHierarchy::GetLCA(GStrIdx str1, GStrIdx str2) const
{
    Klass *result = GetLCA(GetKlassFromStrIdx(str1), GetKlassFromStrIdx(str2));
    return (result != nullptr ? result->GetKlassNameStrIdx() : GStrIdx(0));
}

const std::string &KlassHierarchy::GetLCA(const std::string &name1, const std::string &name2) const
{
    Klass *result = GetLCA(GetKlassFromName(name1), GetKlassFromName(name2));
    return (result != nullptr ? result->GetKlassName() : GlobalTables::GetStrTable().GetStringFromStrIdx(GStrIdx(0)));
}

void KlassHierarchy::AddKlasses()
{
    for (MIRType *type : GlobalTables::GetTypeTable().GetTypeTable()) {
#if DEBUG
        if (type != nullptr) {
            MIRTypeKind kd = type->GetKind();
            if (kd == kTypeStructIncomplete || kd == kTypeClassIncomplete || kd == kTypeInterfaceIncomplete)
                LogInfo::MapleLogger() << "Warning: KlassHierarchy::AddKlasses "
                                       << GlobalTables::GetStrTable().GetStringFromStrIdx(type->GetNameStrIdx())
                                       << " INCOMPLETE \n";
        }
#endif
        if (Options::deferredVisit2 && type && (type->IsIncomplete())) {
            GStrIdx stridx = type->GetNameStrIdx();
            std::string strName = GlobalTables::GetStrTable().GetStringFromStrIdx(stridx);
#if DEBUG
            LogInfo::MapleLogger() << "Waring: " << strName << " INCOMPLETE \n";
#endif
            if (strName == namemangler::kClassMetadataTypeName) {
                continue;
            }
        } else if (type == nullptr || (type->GetKind() != kTypeClass && type->GetKind() != kTypeInterface)) {
            continue;
        }
        GStrIdx strIdx = type->GetNameStrIdx();
        Klass *klass = GetKlassFromStrIdx(strIdx);
        if (klass != nullptr) {
            continue;
        }
        auto *stype = static_cast<MIRStructType *>(type);
        klass = GetMempool()->New<Klass>(stype, &alloc);
        strIdx2KlassMap[strIdx] = klass;
    }
}

void KlassHierarchy::ExceptionFlagProp(Klass &klass)
{
    klass.SetExceptionKlass();
    for (Klass *subClass : klass.GetSubKlasses()) {
        DEBUG_ASSERT(subClass != nullptr, "null ptr check!");
        subClass->SetExceptionKlass();
        ExceptionFlagProp(*subClass);
    }
}

static void CollectImplInterfaces(const Klass &klass, std::set<Klass *> &implInterfaceSet)
{
    for (Klass *superKlass : klass.GetSuperKlasses()) {
        if (implInterfaceSet.find(superKlass) == implInterfaceSet.end()) {
            DEBUG_ASSERT(superKlass != nullptr, "null ptr check!");
            if (superKlass->IsInterface()) {
                implInterfaceSet.insert(superKlass);
            }
            CollectImplInterfaces(*superKlass, implInterfaceSet);
        }
    }
    for (Klass *interfaceKlass : klass.GetImplInterfaces()) {
        if (implInterfaceSet.find(interfaceKlass) == implInterfaceSet.end()) {
            implInterfaceSet.insert(interfaceKlass);
            DEBUG_ASSERT(interfaceKlass != nullptr, "null ptr check!");
            CollectImplInterfaces(*interfaceKlass, implInterfaceSet);
        }
    }
}

void KlassHierarchy::UpdateImplementedInterfaces()
{
    for (const auto &pair : strIdx2KlassMap) {
        Klass *klass = pair.second;
        DEBUG_ASSERT(klass != nullptr, "null ptr check");
        if (!klass->IsInterface()) {
            std::set<Klass *> implInterfaceSet;
            CollectImplInterfaces(*klass, implInterfaceSet);
            for (auto interface : implInterfaceSet) {
                // Add missing parent interface to class link
                interface->AddImplKlass(klass);
                klass->AddImplInterface(interface);
            }
        }
    }
}

void KlassHierarchy::GetParentKlasses(const Klass &klass, std::vector<Klass *> &parentKlasses) const
{
    for (Klass *superKlass : klass.GetSuperKlasses()) {
        parentKlasses.push_back(superKlass);
    }
    if (!klass.IsInterface()) {
        for (Klass *iklass : klass.GetImplInterfaces()) {
            parentKlasses.push_back(iklass);
        }
    }
}

void KlassHierarchy::GetChildKlasses(const Klass &klass, std::vector<Klass *> &childKlasses) const
{
    for (Klass *subKlass : klass.GetSubKlasses()) {
        childKlasses.push_back(subKlass);
    }
    if (klass.IsInterface()) {
        for (Klass *implKlass : klass.GetImplKlasses()) {
            childKlasses.push_back(implKlass);
        }
    }
}

void KlassHierarchy::TopologicalSortKlasses()
{
    std::set<Klass *> inQueue;  // Local variable, no need to use MapleSet
    for (const auto &pair : strIdx2KlassMap) {
        Klass *klass = pair.second;
        DEBUG_ASSERT(klass != nullptr, "klass can not be nullptr");
        if (!klass->HasSuperKlass() && !klass->ImplementsKlass()) {
            topoWorkList.push_back(klass);
            inQueue.insert(klass);
        }
    }
    // Top-down iterates all nodes
    for (size_t i = 0; i < topoWorkList.size(); ++i) {
        Klass *klass = topoWorkList[i];
        std::vector<Klass *> childklasses;
        DEBUG_ASSERT(klass != nullptr, "null ptr check!");
        GetChildKlasses(*klass, childklasses);
        for (Klass *childklass : childklasses) {
            if (inQueue.find(childklass) == inQueue.end()) {
                // callee has not been visited
                bool parentKlassAllVisited = true;
                std::vector<Klass *> parentKlasses;
                DEBUG_ASSERT(childklass != nullptr, "null ptr check!");
                GetParentKlasses(*childklass, parentKlasses);
                // Check whether all callers of the current callee have been visited
                for (Klass *parentKlass : parentKlasses) {
                    if (inQueue.find(parentKlass) == inQueue.end()) {
                        parentKlassAllVisited = false;
                        break;
                    }
                }
                if (parentKlassAllVisited) {
                    topoWorkList.push_back(childklass);
                    inQueue.insert(childklass);
                }
            }
        }
    }
}

void KlassHierarchy::CountVirtualMethods() const
{
    // Bottom-up iterates all klass nodes
    for (size_t i = topoWorkList.size(); i != 0; --i) {
        topoWorkList[i - 1]->CountVirtMethBottomUp();
    }
}

Klass *KlassHierarchy::AddClassFlag(const std::string &name, uint32 flag)
{
    Klass *refKlass = GetKlassFromLiteral(name);
    if (refKlass != nullptr) {
        refKlass->SetFlag(flag);
    }
    return refKlass;
}

void KlassHierarchy::Dump() const
{
    for (Klass *klass : topoWorkList) {
        klass->Dump();
    }
}

GStrIdx KlassHierarchy::GetUniqueMethod(GStrIdx vfuncNameStridx) const
{
    auto it = vfunc2RfuncMap.find(vfuncNameStridx);
    return (it != vfunc2RfuncMap.end() ? it->second : GStrIdx(0));
}

bool KlassHierarchy::IsDevirtualListEmpty() const
{
    return vfunc2RfuncMap.empty();
}

void KlassHierarchy::DumpDevirtualList(const std::string &outputFileName) const
{
    std::unordered_map<std::string, std::string> funcMap;
    for (Klass *klass : topoWorkList) {
        for (MIRFunction *func : klass->GetMethods()) {
            MIRFunction *uniqCand = klass->GetUniqueMethod(func->GetBaseFuncNameWithTypeStrIdx());
            if (uniqCand != nullptr) {
                funcMap[func->GetName()] = uniqCand->GetName();
            }
        }
    }
    std::ofstream outputFile;
    outputFile.open(outputFileName);
    for (auto it : funcMap) {
        outputFile << it.first << "\t" << it.second << "\n";
    }
    outputFile.close();
}

void KlassHierarchy::ReadDevirtualList(const std::string &inputFileName)
{
    std::ifstream inputFile;
    inputFile.open(inputFileName);
    std::string vfuncName;
    std::string rfuncName;
    while (inputFile >> vfuncName >> rfuncName) {
        vfunc2RfuncMap[GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(vfuncName)] =
            GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(rfuncName);
    }
    inputFile.close();
}

void KlassHierarchy::BuildHierarchy()
{
    // Scan class list and generate Klass without method information
    AddKlasses();
    // In the case of "class C implements B; interface B extends A;",
    // we need to add a link between C and A.
    UpdateImplementedInterfaces();
    TopologicalSortKlasses();
    // Use --dump-devirtual-list=... to dump a closed-wolrd analysis result into a file
    if (!Options::dumpDevirtualList.empty()) {
        DumpDevirtualList(Options::dumpDevirtualList);
    }
    // Use --read-devirtual-list=... to read in a closed-world analysis result
    if (!Options::readDevirtualList.empty()) {
        ReadDevirtualList(Options::readDevirtualList);
    }
}

KlassHierarchy::KlassHierarchy(MIRModule *mirmodule, MemPool *memPool)
    : AnalysisResult(memPool),
      alloc(memPool),
      mirModule(mirmodule),
      strIdx2KlassMap(std::less<GStrIdx>(), alloc.Adapter()),
      vfunc2RfuncMap(std::less<GStrIdx>(), alloc.Adapter()),
      topoWorkList(alloc.Adapter())
{
}
}  // namespace maple
