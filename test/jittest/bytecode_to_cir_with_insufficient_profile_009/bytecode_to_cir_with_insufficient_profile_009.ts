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

const v1 = [-1561850355, -8980, -65536];
const v2 = [-6, -536870912, -7, 9019, 9007199254740991];
const v23 = 191;
let o27 = {
    "maxByteLength": 4294967296,
};
const v29 = new SharedArrayBuffer(512, o27);

function Test() {
    const v31 = new BigUint64Array(v29);
    const v42 = v1["filter"](Array, v23);
    try { v42(v31); } catch (e) { }
    const v44 = new Array(57);
    for (const v47 of v44) {
        for (let v58 = 0; v58 < 27; v58++) {
            v2[512];
        }
    }
}

Test();
ArkTools.jitCompileAsync(Test);
print(ArkTools.waitJitCompileFinish(Test));
Test();
