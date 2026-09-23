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

let setterReceiver = null;
let setterValue = -1;

let receiver = {};
Object.defineProperty(receiver, "value", {
    configurable: true,
    set(value) {
        setterReceiver = this;
        setterValue = value;
    }
});

function storeOwnAccessor(object, value) {
    object.value = value;
    return value;
}

for (let i = 0; i < 20000; i++) {
    storeOwnAccessor(receiver, i);
}

ArkTools.arkSteedCompileSync(storeOwnAccessor);
ArkTools.waitJitCompileFinish(storeOwnAccessor);
let ok = ArkTools.isAOTCompiled(storeOwnAccessor);

ok = ok && storeOwnAccessor(receiver, 42) === 42;
ok = ok && setterReceiver === receiver && setterValue === 42;

print(ok ? "PASS" : "FAIL");
