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

function f24() {
}
class C29 extends f24 {
    constructor() {
        super();
        try {
            const v33 = new Proxy(C29, C29);
            try {
                new v33();
            } catch (e35) {
            }
        } catch (e36) {
        }
    }
}
try {
    C29();
} catch(e48) {
}
new C29();
print(1);