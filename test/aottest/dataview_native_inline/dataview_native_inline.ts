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
declare function print(arg: any, arg1?: any): string;

function test1() {
    const v48 = new SharedArrayBuffer(2);
    const v52 = new DataView(v48);
    for (let i40 = 2147483644, i41 = 2147483647; i40 < i41;
        (() => {
            i40++;
            try { v52.setInt8(i41, 2); } catch (e) { }
            return i41;
        })()) {
    }
}
test1();
print(ArkTools.isAOTCompiled(test1));