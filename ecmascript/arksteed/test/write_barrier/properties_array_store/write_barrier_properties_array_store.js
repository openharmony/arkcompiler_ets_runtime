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

function makeHolder() {
    let holder = {};
    holder.a = 0;
    holder.b = 1;
    holder.c = 2;
    holder.d = 3;
    holder.e = 4;
    holder.value = null;
    return holder;
}

function storePropertiesArray(holder, value) {
    holder.value = value;
    return 1;
}

let holder = makeHolder();
let ok = ArkTools.getInlinedPropertiesCount(holder) < 6;
for (let i = 0; i < 20000; i++) {
    storePropertiesArray(holder, { marker: i });
}

ok = ok && ArkTools.getICState(storePropertiesArray, 0, 1) === "mono";
ArkTools.arkSteedCompileSync(storePropertiesArray);
ArkTools.waitJitCompileFinish(storePropertiesArray);
ok = ok && ArkTools.isAOTCompiled(storePropertiesArray);

for (let i = 0; i < 2048; i++) {
    let value = { marker: i };
    storePropertiesArray(holder, value);
    if ((i & 127) === 0) {
        ArkTools.gc();
    }
    if (holder.value !== value || holder.value.marker !== i) {
        ok = false;
        break;
    }
}

print(ok ? "PASS" : "FAIL");
