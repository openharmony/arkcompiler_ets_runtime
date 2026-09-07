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
//! METHOD loadDifferentReceiver
//! COUNT_GE LoadTaggedElement 2
//! HAS_NOT CallCommonStub GetPropertyByValue
declare function print(arg: any): string;

function loadDifferentReceiver(left, right, index)
{
    return left[index] + right[index];
}

const left = [10, 20, 30, 40];
const right = [100, 200, 300, 400];
for (let i = 0; i < 1000; i++) {
    loadDifferentReceiver(left, right, i & 3);
}

ArkTools.arkSteedCompileSync(loadDifferentReceiver);
ArkTools.waitJitCompileFinish(loadDifferentReceiver);

let ok = ArkTools.isAOTCompiled(loadDifferentReceiver);
ok = ok && loadDifferentReceiver(left, right, 1) === 220;

print(ok ? "PASS" : "FAIL");
