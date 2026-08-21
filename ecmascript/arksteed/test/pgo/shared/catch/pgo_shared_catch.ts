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
class CatchSharedHolder {
    value: number = 0;

    constructor(value: number) {
        "use sendable";
        this.value = value;
    }
}

function storeShared(holder: CatchSharedHolder, value: any): string {
    holder.value = value;
    return "stored";
}

function callStoreWithCatch(holder: CatchSharedHolder, value: any): string {
    try {
        return storeShared(holder, value);
    } catch (error) {
        return "caught";
    }
}

let holder = new CatchSharedHolder(0);
for (let i = 0; i < 20000; i++) {
    storeShared(holder, i);
}

ArkTools.arkSteedCompileSync(storeShared);
ArkTools.waitJitCompileFinish(storeShared);
let ok = ArkTools.isAOTCompiled(storeShared);
ok = ok && callStoreWithCatch(holder, 7) === "stored" && holder.value === 7;
ok = ok && callStoreWithCatch(holder, { invalid: true }) === "caught";
ok = ok && holder.value === 7;
print(ok ? "PASS" : "FAIL");
