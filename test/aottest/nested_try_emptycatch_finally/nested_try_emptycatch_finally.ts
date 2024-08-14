/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

class C2 {
    constructor(a3) {
        const v8 = a3.proto;
        try {
            try { v8(); } catch (e) {
            }
        } catch(e17) {
        } finally {
            print("aaa");
        }
    }
}
new C2(1073741824);
print(ArkTools.isAOTCompiled(C2));