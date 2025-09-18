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

const BASE_COUNT = 400

function isByteLength(str, options) {
    var min = options.min || 0;
    var max = options.max;
    var len = encodeURI(str).split(/%..|./).length - 1;
    return len >= min && (typeof max === 'undefined' || len <= max);
}

function Test() {
    let str = `126819368`;
    let result
    for (let i = 0; i < BASE_COUNT; i++) {
        result = isByteLength(str, { min: 1, max: 10 })
    }

    let str2 = 'Unlessssss'
    for (let i = 0; i < BASE_COUNT; i++) {
        result = isByteLength(str2, { min: 2 })
    }

    for (let i = 0; i < 20000; i++) {
        result = isByteLength(str2, { max: 2 })
    }
}

Test()
ArkTools.jitCompileAsync(isByteLength);
print(ArkTools.waitJitCompileFinish(isByteLength));
Test()
