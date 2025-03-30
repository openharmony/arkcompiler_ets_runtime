/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

function Test(options) {
    if ((options === null || options === void 0 ? void 0 : options.eui) === '48') {
        return 1;
    }
    if ((options === null || options === void 0 ? void 0 : options.eui) === '64') {
        return 2;
    }
    return Test({eui: '48'}) || Test({eui: '64'});
}

Test({no_separators: false});
ArkTools.jitCompileAsync(Test);
ArkTools.waitJitCompileFinish(Test);
print(Test({no_separators: false}));
