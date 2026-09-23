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
//! PARAMS --compiler-jit-hotness-threshold=6000
//! METHOD readGlobalVar
//! HAS HeapConstant
//! HAS LoadTaggedField
//! HAS DeoptIfTaggedCondition
//! HAS_NOT CallCommonStub LdGlobalVar

var gLoad = 1;

function readGlobalVar(x) {
    return gLoad + x;
}

for (let i = 0; i < 20000; i++) {
    readGlobalVar(i);
}

ArkTools.arkSteedCompileSync(readGlobalVar);
let ok = true;

// The inlined PropertyBox load must observe updates made through the global
// object: the JIT store and the globalThis write share the same box.
globalThis.gLoad = 100;
for (let i = 0; i < 2048; i++) {
    ok = ok && readGlobalVar(i) === 100 + i;
    if ((i & 127) === 0) {
        ArkTools.gc();
    }
}

print(ok ? "PASS" : "FAIL");
