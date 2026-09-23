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

let setterValue = -1;
let shouldThrow = false;

let proto = {};
Object.defineProperty(proto, "value", {
    configurable: true,
    set(value) {
        if (shouldThrow) {
            throw new Error("setter failure");
        }
        setterValue = value;
    }
});

function storeAccessor(receiver, value) {
    receiver.value = value;
    return value;
}

let receiver = Object.create(proto);
for (let i = 0; i < 20000; i++) {
    storeAccessor(receiver, i);
}

ArkTools.arkSteedCompileSync(storeAccessor);
ArkTools.waitJitCompileFinish(storeAccessor);
let ok = ArkTools.isAOTCompiled(storeAccessor);

let caught = false;
shouldThrow = true;
try {
    storeAccessor(receiver, 777);
} catch (error) {
    caught = error.message === "setter failure";
}
ok = ok && caught && setterValue === 19999;

print(ok ? "PASS" : "FAIL");
