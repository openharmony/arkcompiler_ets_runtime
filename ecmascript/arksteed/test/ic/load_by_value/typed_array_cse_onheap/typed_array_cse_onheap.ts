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
//! METHOD loadTwice
//! COUNT TypedArrayIntLoadElement 1
//! COUNT LoadInt32Field 1
//! HAS LoadTaggedField
//! HAS DeoptIfHClassMismatch
//! HAS DeoptIfInt32Condition
//! HAS_NOT CallCommonStub GetPropertyByValue
declare function print(arg: any): string;

function loadTwice(value, index)
{
    return value[index] + value[index];
}

const values = new Uint8Array([1, 2, 3, 4]);
for (let i = 0; i < 1000; i++) {
    loadTwice(values, i & 3);
}

ArkTools.arkSteedCompileSync(loadTwice);
ArkTools.waitJitCompileFinish(loadTwice);

let ok = ArkTools.isAOTCompiled(loadTwice);
ok = ok && ArkTools.isOnHeap(values);
ok = ok && loadTwice(values, 1) === 4;
ok = ok && loadTwice(values, 3) === 8;

print(ok ? "PASS" : "FAIL");
