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

#include "mdrecord.h"

namespace MDGen {
constexpr unsigned int kInValidStrIdx = UINT_MAX;

bool DefTyElement::SetContent(const StrInfo curInfo, const std::set<unsigned int> &childTySet)
{
    if (!childTySet.count(curInfo.idx)) {
        return false;
    }
    elementIdx = curInfo.idx;
    return true;
}

bool DefObjElement::SetContent(const StrInfo curInfo, const MDClass &parentClass)
{
    if (!parentClass.IsClassMember(curInfo.idx)) {
        return false;
    }
    elementIdx = curInfo.idx;
    return true;
}

const MDElement *MDObject::GetOneMDElement(size_t index) const
{
    CHECK_FATAL(index < mdElements.size(), "Array boundary check failed");
    return mdElements[index];
}

const MDObject &MDClass::GetOneMDObject(size_t index) const
{
    CHECK_FATAL(index < mdObjects.size(), "Array boundary check failed");
    return mdObjects[index];
}

void MDClass::AddClassMember(MDObject inputObj)
{
    mdObjects.emplace_back(inputObj);
    (void)childObjNames.insert(inputObj.GetIdx());
}

bool MDClass::IsClassMember(unsigned int curIdx) const
{
    return childObjNames.count(curIdx);
}

void MDClass::BuildFormalTypes(unsigned int memberIdx, bool isVec)
{
    formalTypes.emplace_back(std::make_pair(memberIdx, isVec));
}

bool MDClass::IsValidStructEle(RecordType curTy) const
{
    return (curTy == kTypeName || curTy == kClassName || curTy == kIntType || curTy == kStringType);
}

unsigned int MDClassRange::CreateStrInTable(const std::string &inStr, RecordType curTy)
{
    unsigned int result = kInValidStrIdx;
    StrInfo curInfo(totalStr, curTy);
    auto ret = stringHashTable.insert(std::make_pair(inStr, curInfo));
    if (ret.second) {
        unsigned int temp = totalStr;
        stringTable.emplace_back(inStr);
        ++totalStr;
        return temp;
    }
    return result;
}

StrInfo MDClassRange::GetStrInTable(const std::string &inStr)
{
    auto ret = stringHashTable.find(inStr);
    StrInfo inValidInfo(UINT_MAX, kUndefinedStr);
    return (ret != stringHashTable.end()) ? ret->second : inValidInfo;
}

RecordType MDClassRange::GetStrTyByIdx(size_t curIdx)
{
    CHECK_FATAL(curIdx < stringTable.size(), "Array boundary check failed");
    return GetStrInTable(stringTable[curIdx]).sType;
}

const std::string &MDClassRange::GetStrByIdx(size_t curIdx)
{
    CHECK_FATAL(curIdx < stringTable.size(), "Array boundary check failed");
    return stringTable[curIdx];
}

void MDClassRange::ModifyStrTyInTable(const std::string &inStr, RecordType newTy)
{
    auto ret = stringHashTable.find(inStr);
    CHECK_FATAL(ret != stringHashTable.end(), "find string failed!");
    ret->second.sType = newTy;
}

void MDClassRange::AddDefinedType(unsigned int typesName, std::set<unsigned int> typesSet)
{
    (void)definedTypes.insert(std::make_pair(typesName, typesSet));
}

void MDClassRange::AddMDClass(MDClass curClass)
{
    (void)allClasses.insert(std::make_pair(curClass.GetClassIdx(), curClass));
}

void MDClassRange::FillMDClass(unsigned int givenIdx, const MDObject &insertObj)
{
    auto ret = allClasses.find(givenIdx);
    CHECK_FATAL(ret != allClasses.end(), "Cannot achieve target MD Class");
    ret->second.AddClassMember(insertObj);
}

MDClass MDClassRange::GetOneMDClass(unsigned int givenIdx)
{
    auto ret = allClasses.find(givenIdx);
    CHECK_FATAL(ret != allClasses.end(), "Cannot achieve target MD Class");
    return ret->second;
}

std::set<unsigned int> MDClassRange::GetOneSpcType(unsigned int givenTyIdx)
{
    auto ret = definedTypes.find(givenTyIdx);
    CHECK_FATAL(ret != definedTypes.end(), "Cannot achieve a defined type");
    return ret->second;
}
} /* namespace MDGen */
