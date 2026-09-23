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
//! METHOD storeThenLoad
//! COUNT_LE LoadTaggedElement 2
//! HAS StoreTaggedElement
//! HAS_NOT CallCommonStub GetPropertyByValue
declare function print(arg: any): string;

function storeThenLoad(array, index, value)
{
    let first = array[index];
    array[index] = value;
    return first + array[index];
}

const values = [1, 2, 3, 4];
for (let i = 0; i < 1000; i++) {
    storeThenLoad(values, 1, i);
}

ArkTools.arkSteedCompileSync(storeThenLoad);
ArkTools.waitJitCompileFinish(storeThenLoad);

let ok = ArkTools.isAOTCompiled(storeThenLoad);
values[1] = 2;
ok = ok && storeThenLoad(values, 1, 20) === 22;
ok = ok && values[1] === 20;

print(ok ? "PASS" : "FAIL");
