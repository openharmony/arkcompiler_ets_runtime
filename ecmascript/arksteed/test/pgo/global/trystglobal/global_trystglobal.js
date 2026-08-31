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

//! PARAMS --compiler-jit-hotness-threshold=6000
//! METHOD writeGlobalVar
//! HAS StoreTaggedFieldWithBarrier
//! HAS_NOT CallCommonStub TryStGlobalByName

globalThis.gStore = 0;

function writeGlobalVar(v) {
    gStore = v;
    return gStore;
}

for (let i = 0; i < 20000; i++) {
    writeGlobalVar(i);
}

ArkTools.arkSteedCompileSync(writeGlobalVar);
let ok = true;

for (let i = 0; i < 2048; i++) {
    ok = ok && writeGlobalVar(i) === i;
    if ((i & 127) === 0) {
        ArkTools.gc();
    }
}
ok = ok && globalThis.gStore === 2047;

// Redefining the binding as an accessor invalidates the cached PropertyBox; the inlined
// store must deopt and the setter must take over.
let seen = -1;
Object.defineProperty(globalThis, "gStore", {
    set: function (v) { seen = v; },
    get: function () { return 7; },
    configurable: true
});
writeGlobalVar(42);
ok = ok && seen === 42 && globalThis.gStore === 7;

print(ok ? "PASS" : "FAIL");
