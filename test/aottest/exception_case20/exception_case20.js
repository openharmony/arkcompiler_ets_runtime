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

const v4 = new WeakSet();
class C15 {}
try {
    new C15()
} catch(e17) {
}
const v18 = new C15(C15, "description", "description");
const o35 = {};
o35.c = 268435456;
const v42 = new Proxy(v18, o35, C15);

try {
    async function f45(a46, a47, a48) {
        const v52 = [v4, a46];
        const v53 = Math.atan2;
        try {
            v53(a46, a47);
        } catch (e55) {

        }
        Reflect.apply(v53, v42, v52);
        const v57 = await Math;
        return C15;
    }
    const v60 = f45(255, "description", 255);
} catch(e67) {
}
print(1)