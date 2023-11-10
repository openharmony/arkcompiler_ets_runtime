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

#ifndef MAPLEBE_MDGEN_INCLUDE_MDPARSER_H
#define MAPLEBE_MDGEN_INCLUDE_MDPARSER_H

#include "mdlexer.h"
#include "mdrecord.h"
#include "mempool.h"

namespace MDGen {
class MDParser {
public:
    MDParser(MDClassRange &newKeeper, maple::MemPool *memPool) : dataKeeper(newKeeper), mdMemPool(memPool) {}
    ~MDParser() = default;

    bool ParseFile(const std::string &inputFile);
    bool ParseObjectStart();
    bool ParseObject();
    bool IsObjectStart(MDTokenKind k) const;
    bool ParseDefType();
    bool ParseMDClass();
    bool ParseMDClassBody(MDClass &oneClass);
    bool ParseMDObject();
    bool ParseMDObjBody(MDObject &curObj);
    bool ParseIntElement(MDObject &curObj, bool isVec);
    bool ParseStrElement(MDObject &curObj, bool isVec);
    bool ParseDefTyElement(MDObject &curObj, bool isVec, const std::set<unsigned int> &childSet);
    bool ParseDefObjElement(MDObject &curObj, bool isVec, const MDClass &pClass);

    /* error process */
    bool EmitError(const std::string &errMsg);

private:
    MDLexer lexer;
    MDClassRange &dataKeeper;
    maple::MemPool *mdMemPool;
};
} /* namespace MDGen */

#endif /* MAPLEBE_MDGEN_INCLUDE_MDPARSER_H */