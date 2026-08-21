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

function storePrototypeTransition(holder, value) {
    holder.added = value;
    return holder.added;
}

let prototypeHolders = [];
for (let i = 0; i < 257; i++) {
    class Holder {}
    prototypeHolders.push(Holder.prototype);
}

storePrototypeTransition(prototypeHolders[0], 0);

ArkTools.arkSteedCompileSync(storePrototypeTransition);
ArkTools.waitJitCompileFinish(storePrototypeTransition);
let ok = ArkTools.isAOTCompiled(storePrototypeTransition);
for (let i = 0; i < 256; i++) {
    let holder = prototypeHolders[i + 1];
    ok = ok && storePrototypeTransition(holder, i) === i;
    ok = ok && holder.added === i;
}

print(ok ? "PASS" : "FAIL");
