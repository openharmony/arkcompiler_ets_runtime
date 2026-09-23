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

let receiver = {};
Object.defineProperty(receiver, "value", {
    configurable: true,
    get() {
        return 1;
    }
});

function storeUndefinedSetter(object, value) {
    object.value = value;
    return value;
}

for (let i = 0; i < 20000; i++) {
    try {
        storeUndefinedSetter(receiver, i);
    } catch (error) {
    }
}

ArkTools.arkSteedCompileSync(storeUndefinedSetter);
ArkTools.waitJitCompileFinish(storeUndefinedSetter);
let ok = ArkTools.isAOTCompiled(storeUndefinedSetter);

let caught = false;
try {
    storeUndefinedSetter(receiver, 42);
} catch (error) {
    caught = error instanceof TypeError;
}
ok = ok && caught;

print(ok ? "PASS" : "FAIL");
