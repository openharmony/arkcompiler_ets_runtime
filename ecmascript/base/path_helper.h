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
#ifndef ECMASCRIPT_BASE_PATH_HELPER_H
#define ECMASCRIPT_BASE_PATH_HELPER_H

#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/jspandafile/js_pandafile.h"

namespace panda::ecmascript::base {
class PathHelper {
public:
    static constexpr std::string_view COLON_TAG = ":";
    static constexpr std::string_view CURRENT_DIREATORY_TAG = "./";
    static constexpr std::string_view DOUBLE_POINT_TAG = "..";
    static constexpr std::string_view DOUBLE_SLASH_TAG = "//";
    static constexpr char NAME_SPACE_TAG = '@';
    static constexpr char NORMALIZED_OHMURL_TAG = '&';
    static constexpr std::string_view POINT_STRING_TAG = ".";
    static constexpr char POINT_TAG = '.';
    static constexpr char SLASH_TAG = '/';

    static CString NormalizePath(const CString &fileName);
    static CString ResolveDirPath(const CString &fileName);

    /*
     * Before: moduleName@nameSpace
     * After:  moduleName
     */
    inline static void DeleteNamespace(CString &moduleName)
    {
        size_t pos = moduleName.find(NAME_SPACE_TAG);
        if (pos == CString::npos) {
            return;
        }
        moduleName.erase(pos, moduleName.size() - pos);
    }

    /*
     * Before: moduleName@nameSpace
     * After:  nameSpace
     */
    inline static CString GetHarName(const CString &moduleName)
    {
        size_t pos = moduleName.find(NAME_SPACE_TAG);
        ASSERT(pos != CString::npos);
        return moduleName.substr(pos + 1);
    }

    /*
     * Before: bundleName/moduleName@namespace/moduleName/xxx/xxx
     * After:  moduleName/xxx/xxx
     */
    inline static void AdaptOldIsaRecord(CString &recordName)
    {
        size_t pos = recordName.find(SLASH_TAG);
        if (pos != CString::npos) {
            pos = recordName.find(SLASH_TAG, pos + 1);
            if (pos != CString::npos) {
                recordName = recordName.substr(pos + 1);
            }
        }
    }

    /*
     * Before: @***:xxxx
     * After:  xxxx
     */
    inline static CString GetStrippedModuleName(const CString &moduleRequestName)
    {
        size_t pos = moduleRequestName.find(COLON_TAG);
        if (pos == CString::npos) {
            LOG_FULL(FATAL) << "Unknown format " << moduleRequestName;
        }
        return moduleRequestName.substr(pos + 1);
    }

    /*
     * Before: @xxx:****
     * After:  xxx
     */
    inline static CString GetInternalModulePrefix(const CString &moduleRequestName)
    {
        size_t pos = moduleRequestName.find(COLON_TAG);
        if (pos == CString::npos) {
            LOG_FULL(FATAL) << "Unknown format " << moduleRequestName;
        }
        ASSERT(pos != 0);
        return moduleRequestName.substr(1, pos - 1);
    }
};
}  // namespace panda::ecmascript::base
#endif  // ECMASCRIPT_BASE_PATH_HELPER_H