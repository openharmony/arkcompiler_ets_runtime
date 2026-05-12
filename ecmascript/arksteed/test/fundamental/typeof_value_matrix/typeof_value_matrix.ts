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
function typeof_value_matrix() {
    let undef;
    let booleanValue = true;
    let numberValue = 1;
    let bigintValue = 2n;
    let stringValue = "x";
    let symbolValue = Symbol("y");
    let objectValue = { z: 1 };

    print(typeof undef);
    print(typeof booleanValue);
    print(typeof numberValue);
    print(typeof bigintValue);
    print(typeof stringValue);
    print(typeof symbolValue);
    print(typeof objectValue);
}

ArkTools.arkSteedCompileAsync(typeof_value_matrix);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

typeof_value_matrix();
