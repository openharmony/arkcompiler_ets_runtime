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

//! PARAMS --enable-force-gc=true
//! METHOD asyncHeapConstant
//! HAS HeapConstant

function asyncHeapConstant(value) {
    return "async-heap-constant:" + value;
}

for (let i = 0; i < 32; i++) {
    asyncHeapConstant(i);
}

let ok = ArkTools.arkSteedCompileAsync(asyncHeapConstant);

// The compiler thread records handles before the host thread installs code.
// Repeated collections here exercise that interval and force installation to
// resolve the current handle value instead of using a compile-time address.
for (let round = 0; round < 4; round++) {
    let garbage = [];
    for (let i = 0; i < 512; i++) {
        garbage.push({ round: round, index: i, payload: "heap-constant-gc-pressure" });
    }
    ArkTools.forceFullGC();
}

ArkTools.waitJitCompileFinish(asyncHeapConstant);
ok = ok && ArkTools.isAOTCompiled(asyncHeapConstant);
ok = ok && asyncHeapConstant(17) === "async-heap-constant:17";

ok = ok && asyncHeapConstant(27) === "async-heap-constant:27";

print(ok ? "PASS" : "FAIL");
