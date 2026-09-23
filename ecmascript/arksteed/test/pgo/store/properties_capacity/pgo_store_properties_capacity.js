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

function makeCapacityBase() {
    let holder = {};
    holder.a = 0;
    holder.b = 1;
    holder.c = 2;
    holder.d = 3;
    holder.e = 4;
    holder.f = 5;
    holder.g = 6;
    holder.h = 7;
    return holder;
}

function storeWithCapacityGrowth(holder, value) {
    holder.added = value;
    return holder.added;
}

for (let i = 0; i < 20000; i++) {
    storeWithCapacityGrowth(makeCapacityBase(), { marker: i });
}

ArkTools.arkSteedCompileSync(storeWithCapacityGrowth);
ArkTools.waitJitCompileFinish(storeWithCapacityGrowth);
let ok = ArkTools.isAOTCompiled(storeWithCapacityGrowth);

for (let i = 0; i < 1024; i++) {
    let holder = makeCapacityBase();
    let value = { marker: i };
    ok = ok && storeWithCapacityGrowth(holder, value) === value;
    ok = ok && holder.h === 7 && holder.added.marker === i;
    if ((i & 63) === 0) {
        ArkTools.gc();
    }
}

print(ok ? "PASS" : "FAIL");
