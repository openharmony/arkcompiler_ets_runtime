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
#include "ecmascript/base/path_helper.h"

namespace panda::ecmascript::base {
/*
 * Before: ./xxx/../xxx/xxx/
 * After:  xxx/xxx
 */
CString PathHelper::NormalizePath(const CString &fileName)
{
    if (fileName.empty() || (fileName.find(DOUBLE_SLASH_TAG) == CString::npos &&
        fileName.find(CURRENT_DIREATORY_TAG) == CString::npos &&
        fileName[fileName.size() - 1] != SLASH_TAG)) {
        return fileName;
    }
    CString res;
    size_t prev = 0;
    size_t curr = fileName.find(SLASH_TAG);
    CVector<CString> elems;
    // eliminate parent directory path
    while (curr != CString::npos) {
        if (curr > prev) {
            CString elem = fileName.substr(prev, curr - prev);
            if (elem == DOUBLE_POINT_TAG && !elems.empty()) { // looking for xxx/../
                elems.pop_back();
            } else if (elem != POINT_STRING_TAG && elem != DOUBLE_POINT_TAG) { // remove ./ ../
                elems.push_back(elem);
            }
        }
        prev = curr + 1;
        curr = fileName.find(SLASH_TAG, prev);
    }
    if (prev != fileName.size()) {
        elems.push_back(fileName.substr(prev));
    }
    for (auto& e : elems) {
        if (res.empty() && fileName[0] != SLASH_TAG) {
            res.append(e);
            continue;
        }
        res.append(1, SLASH_TAG).append(e);
    }
    return res;
}

/*
 * Before: xxx/xxx
 * After:  xxx/
 */
CString PathHelper::ResolveDirPath(const CString &fileName)
{
    // find last '/', '\\'
    const size_t foundPos = fileName.find_last_of("/\\");
    if (foundPos == CString::npos) {
        return {};
    }
    return fileName.substr(0, foundPos + 1);
}
}  // namespace panda::ecmascript::base