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
//! COUNT LoadTaggedElement 1
//! COUNT LoadTaggedField 1
//! COUNT LoadInt32Field 1
//! HAS_NOT CallCommonStub GetPropertyByValue

//! METHOD loadTwiceAfterCall
//! COUNT_LE LoadTaggedElement 2
//! HAS_NOT CallCommonStub GetPropertyByValue
declare function print(arg: any): string;

function loadTwice(array, index)
{
    return array[index] + array[index];
}

function loadTwiceAfterCall(array, index, callback)
{
    let first = array[index];
    callback(array);
    return first + array[index];
}

function noOp(array)
{
    return array;
}

const values = [10, 20, 30, 40];
for (let i = 0; i < 1000; i++) {
    loadTwice(values, i & 3);
    loadTwiceAfterCall(values, i & 3, noOp);
}

ArkTools.arkSteedCompileSync(loadTwice);
ArkTools.arkSteedCompileSync(loadTwiceAfterCall);
ArkTools.waitJitCompileFinish(loadTwice);
ArkTools.waitJitCompileFinish(loadTwiceAfterCall);

let ok = ArkTools.isAOTCompiled(loadTwice);
ok = ok && ArkTools.isAOTCompiled(loadTwiceAfterCall);
ok = ok && loadTwice(values, 2) === 60;
ok = ok && loadTwiceAfterCall(values, 1, noOp) === 40;

print(ok ? "PASS" : "FAIL");
