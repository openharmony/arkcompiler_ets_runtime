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

//! COMPILE_MODE script
//! PARAMS --compiler-jit-hotness-threshold=10000
//! METHOD readTwice
//! HAS HeapConstant
//! COUNT LoadTaggedField 1
//! COUNT_LE DeoptIfTaggedCondition 2
//! HAS_NOT CallCommonStub LdGlobalVar

var globalReadValue = 10;

function readTwice()
{
    return globalReadValue + globalReadValue;
}

for (let i = 0; i < 1000; i++) {
    readTwice();
}

ArkTools.arkSteedCompileSync(readTwice);
ArkTools.waitJitCompileFinish(readTwice);

let ok = ArkTools.isAOTCompiled(readTwice);
globalThis.globalReadValue = 20;
for (let i = 0; i < 2048; i++) {
    ok = ok && readTwice() === 40;
    if ((i & 127) === 0) {
        ArkTools.gc();
    }
}

print(ok ? "PASS" : "FAIL");
