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
//! METHOD loadAroundMutation
//! COUNT_LE TypedArrayIntLoadElement 2
//! COUNT_LE LoadInt32Field 2
//! HAS DeoptIfHClassMismatch
//! COUNT_LE DeoptIfInt32Condition 2
//! HAS_NOT CallCommonStub GetPropertyByValue

//! METHOD loadAroundDetach
//! COUNT_LE TypedArrayIntLoadElement 2
//! COUNT_LE LoadInt32Field 2
//! HAS DeoptIfHClassMismatch
//! COUNT_LE DeoptIfInt32Condition 2
//! COUNT_LE DeoptIfTaggedCondition 2
//! HAS_NOT CallCommonStub GetPropertyByValue
declare function print(arg: any): string;

function loadAroundMutation(value, index, callback)
{
    let first = value[index];
    callback(value);
    return first + value[index];
}

function loadAroundDetach(value, index, callback)
{
    let first = value[index];
    callback(value);
    return first + value[index];
}

function mutate(value)
{
    value[1] = 20;
}

function noOp(value)
{
    return value;
}

function detach(value)
{
    ArkTools.arrayBufferDetach(value.buffer);
}

const mutated = new Uint8Array([1, 2, 3, 4]);
const detached = new Uint8Array(5000);
detached[1] = 6;
for (let i = 0; i < 1000; i++) {
    loadAroundMutation(mutated, 1, mutate);
    loadAroundDetach(detached, 1, noOp);
}

ArkTools.arkSteedCompileSync(loadAroundMutation);
ArkTools.arkSteedCompileSync(loadAroundDetach);

mutated[1] = 2;
let ok = loadAroundMutation(mutated, 1, mutate) === 22;

let threwTypeError = false;
try {
    loadAroundDetach(detached, 1, detach);
} catch (error) {
    threwTypeError = error instanceof TypeError;
}
ok = ok && threwTypeError;

print(ok ? "PASS" : "FAIL");
