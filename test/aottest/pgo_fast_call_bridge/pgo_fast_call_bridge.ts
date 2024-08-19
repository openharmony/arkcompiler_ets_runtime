/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

function f(prn, a1, a2, a3, a4, a5, a6, a7, a8) {
    let x = a1 + a2;
    if (prn) {
        print(x);
    }
}

let isAot = ArkTools.isAOTCompiled(f);
print(isAot)    // pgo:false, aot:true

let prn = true;
for (let i = 0; i < 100000; i++) {
    f(prn, 1);   // NaN
    f(prn, 1, 2);    // 3
    f(prn, 1, 2, 3); // 3
    f(prn, 1, 2, 3, 4);  // 3
    f(prn, 1, 2, 3, 4, 5);   // 3
    f(prn, 1, 2, 3, 4, 5, 6);    // 3
    f(prn, 1, 2, 3, 4, 5, 6, 7); // 3
    f(prn, 1, 2, 3, 4, 5, 6, 7, 8);  // 3
    f(prn, 1, 2, 3, 4, 5, 6, 7, 8, 9);   // 3
    f(prn, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);   // 3
    prn = false;
    if (!isAot) {
        break;
    }
}

isAot = ArkTools.isAOTCompiled(f);
print(isAot)    // pgo:false, aot:true