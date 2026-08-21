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

//! PARAMS --compiler-enable-jit-lazy-deopt=false

function makeArrayBase() {
    return [1, 2, 3];
}

function storeArrayTransition(holder, value) {
    holder.added = value;
    return holder.added;
}

for (let i = 0; i < 20000; i++) {
    storeArrayTransition(makeArrayBase(), i);
}

ArkTools.arkSteedCompileSync(storeArrayTransition);
ArkTools.waitJitCompileFinish(storeArrayTransition);
let ok = ArkTools.isAOTCompiled(storeArrayTransition);
for (let i = 0; i < 256; i++) {
    let holder = makeArrayBase();
    ok = ok && storeArrayTransition(holder, i) === i;
    ok = ok && holder.length === 3 && holder.added === i;
}

print(ok ? "PASS" : "FAIL");
