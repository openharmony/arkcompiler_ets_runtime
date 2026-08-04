/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// @ts-nocheck
declare function print(value: any): void;

function forceFullGC() {
    ArkTools.forceFullGC();
    if (ArkTools.GC !== undefined) {
        ArkTools.GC.startGC("full", undefined, true);
    }
}

function forceYoungGC() {
    if (ArkTools.GC !== undefined) {
        ArkTools.GC.startGC("young", undefined, true);
        return;
    }
    ArkTools.gc();
}

function storeYoungObject(array, index, marker) {
    array[index] = { marker: marker, payload: [marker, marker + 1] };
}

let array = new Array(3);
array[0] = null;
array[1] = null;
array[2] = null;
for (let i = 0; i < 20000; i++) {
    storeYoungObject(array, 1, i);
}

forceFullGC();
ArkTools.arkSteedCompileSync(storeYoungObject);

let passed = true;
for (let i = 0; i < 128; i++) {
    storeYoungObject(array, 1, i);
    forceYoungGC();
    if (array[1].marker !== i || array[1].payload[1] !== i + 1) {
        passed = false;
        break;
    }
}
print(passed ? "PASS" : "FAIL");
