/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

const v15 = new Int8Array(1263100262);
function f16(a17, a18) {
    const v22 = new ArrayBuffer(102, {});
    v22.constructor = f16;
    const v23 = v22?.constructor;
    try { new v23(a18); } catch (e) {}
    const v25 = new Int8Array(v22);
    return v25;
}
const v27 = Symbol.species;
Object.defineProperty(f16, v27, { writable: true, value: f16});
v15.constructor = f16;
try {
    v15.slice();
} catch (e) {
    print(e);
}
print(1);