/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

// @ts-nocheck
//! PARAMS --compiler-jit-hotness-threshold=10000
//! METHOD loadDifferentIndex
//! COUNT_GE LoadTaggedElement 2
//! HAS_NOT CallCommonStub GetPropertyByValue
declare function print(arg: any): string;

function loadDifferentIndex(array, left, right)
{
    return array[left] + array[right];
}

const values = [10, 20, 30, 40];
for (let i = 0; i < 1000; i++) {
    loadDifferentIndex(values, i & 3, (i + 1) & 3);
}

ArkTools.arkSteedCompileSync(loadDifferentIndex);
ArkTools.waitJitCompileFinish(loadDifferentIndex);

let ok = ArkTools.isAOTCompiled(loadDifferentIndex);
ok = ok && loadDifferentIndex(values, 1, 2) === 50;

print(ok ? "PASS" : "FAIL");
