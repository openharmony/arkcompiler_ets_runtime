/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_OHOS_ADAPTER_MODULEMANAGER_PKG_CONTEXT_STORE_H
#define ECMASCRIPT_OHOS_ADAPTER_MODULEMANAGER_PKG_CONTEXT_STORE_H

#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {

class PkgContextStore {
public:
    void SetPkgNameList(const CMap<CString, CString> &list)
    {
        WriteLockHolder lock(pkgNameListLock_);
        pkgNameList_ = list;
    }

    void UpdatePkgNameList(const CMap<CString, CString> &list)
    {
        WriteLockHolder lock(pkgNameListLock_);
        pkgNameList_.insert(list.begin(), list.end());
    }

    CMap<CString, CString> GetPkgNameList() const
    {
        ReadLockHolder lock(pkgNameListLock_);
        return pkgNameList_;
    }

    CString GetPkgName(const CString &moduleName) const
    {
        ReadLockHolder lock(pkgNameListLock_);
        auto it = pkgNameList_.find(moduleName);
        if (it == pkgNameList_.end()) {
            LOG_ECMA(INFO) << " Get Pkg Name failed";
            return moduleName;
        }
        return it->second;
    }

    void SetPkgAliasList(const CMap<CString, CString> &list)
    {
        WriteLockHolder lock(pkgAliasListLock_);
        pkgAliasList_ = list;
    }

    void UpdatePkgAliasList(const CMap<CString, CString> &list)
    {
        WriteLockHolder lock(pkgAliasListLock_);
        pkgAliasList_.insert(list.begin(), list.end());
    }

    CMap<CString, CString> GetPkgAliasList() const
    {
        ReadLockHolder lock(pkgAliasListLock_);
        return pkgAliasList_;
    }

    CString GetPkgNameWithAlias(const CString &alias) const
    {
        ReadLockHolder lock(pkgAliasListLock_);
        auto it = pkgAliasList_.find(alias);
        if (it == pkgAliasList_.end()) {
            return alias;
        }
        return it->second;
    }

    void SetPkgContextInfoList(const CMap<CString, CMap<CString, CVector<std::pair<CString, CString>>>> &list)
    {
        WriteLockHolder lock(pkgContextInfoLock_);
        pkgContextInfoList_ = list;
    }

    void UpdatePkgContextInfoList(const CMap<CString, CMap<CString, CVector<std::pair<CString, CString>>>> &list)
    {
        WriteLockHolder lock(pkgContextInfoLock_);
        pkgContextInfoList_.insert(list.begin(), list.end());
    }

    CMap<CString, CMap<CString, CVector<std::pair<CString, CString>>>> GetPkgContextInfoList() const
    {
        ReadLockHolder lock(pkgContextInfoLock_);
        return pkgContextInfoList_;
    }

    bool IsNormalizedOhmUrlPack() const
    {
        ReadLockHolder lock(pkgContextInfoLock_);
        return !pkgContextInfoList_.empty();
    }

    void GetPkgContextInfoListElements(const CString &moduleName, const CString &packageName,
                                       CVector<std::pair<CString, CString>> &resultList) const
    {
        ReadLockHolder lock(pkgContextInfoLock_);
        if (packageName.empty()) {
            return;
        }
        auto pkgContextIt = pkgContextInfoList_.find(moduleName);
        if (pkgContextIt == pkgContextInfoList_.end()) {
            return;
        }
        const CMap<CString, CVector<std::pair<CString, CString>>> &pkgList = pkgContextIt->second;
        auto pkgIt = pkgList.find(packageName);
        if (pkgIt == pkgList.end()) {
            return;
        }
        resultList = pkgIt->second;
    }

    void SetOhExportsList(const CUnorderedMap<CString, CUnorderedMap<CString,
        CUnorderedSet<CString>>> &ohExportsMap)
    {
        WriteLockHolder lock(ohExportListLock_);
        ohExportsList_ = ohExportsMap;
    }

    void UpdateOhExportsList(const CUnorderedMap<CString, CUnorderedMap<CString,
        CUnorderedSet<CString>>> &ohExportsMap)
    {
        WriteLockHolder lock(ohExportListLock_);
        ohExportsList_.insert(ohExportsMap.begin(), ohExportsMap.end());
    }

    CUnorderedMap<CString, CUnorderedMap<CString, CUnorderedSet<CString>>> GetOhExportList() const
    {
        ReadLockHolder lock(ohExportListLock_);
        return ohExportsList_;
    }

    bool CheckOhExportsWithOhmurl(const CString &moduleName, const CString &packageName,
                                  const CString &ohmurl) const
    {
        ReadLockHolder lock(ohExportListLock_);
        if (packageName.empty()) {
            return true;
        }
        auto moduleIt = ohExportsList_.find(moduleName);
        if (moduleIt == ohExportsList_.end()) {
            return true;
        }
        const CUnorderedMap<CString, CUnorderedSet<CString>> &moduleExportsList = moduleIt->second;
        auto packageIt = moduleExportsList.find(packageName);
        if (packageIt == moduleExportsList.end()) {
            return true;
        }
        const CUnorderedSet<CString> &packageExportsList = packageIt->second;
        return (packageExportsList.find(ohmurl) != packageExportsList.end());
    }

    // Reset process-level non-form pkg context (main VM teardown / Runtime reuse).
    void ClearPkgContextInfo()
    {
        WriteLockHolder l1(pkgNameListLock_);
        WriteLockHolder l2(pkgAliasListLock_);
        WriteLockHolder l3(pkgContextInfoLock_);
        WriteLockHolder l4(ohExportListLock_);
        pkgNameList_.clear();
        pkgAliasList_.clear();
        pkgContextInfoList_.clear();
        ohExportsList_.clear();
    }

private:
    mutable RWLock pkgContextInfoLock_;
    mutable RWLock pkgAliasListLock_;
    mutable RWLock pkgNameListLock_;
    mutable RWLock ohExportListLock_;
    CMap<CString, CString> pkgNameList_;
    CMap<CString, CMap<CString, CVector<std::pair<CString, CString>>>> pkgContextInfoList_;
    CMap<CString, CString> pkgAliasList_;
    CUnorderedMap<CString, CUnorderedMap<CString, CUnorderedSet<CString>>> ohExportsList_;
};

}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_OHOS_ADAPTER_MODULEMANAGER_PKG_CONTEXT_STORE_H