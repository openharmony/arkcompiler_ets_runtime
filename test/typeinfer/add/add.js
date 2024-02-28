/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

let x = 0
let y = 3.14
let z = x + y
ArkTools.pgoAssertType(z, "number"); // int + int -> int

y = 0
z = x + y
ArkTools.pgoAssertType(z, "int");

x = 3.14
y = 3.14
z = x + y
ArkTools.pgoAssertType(z, "double");

class A {
    constructor(a, b) {
        this.a = 1
        this.b = 2.2
    }
}

let instanceA = new A();

let resultA = instanceA.a + 1;
ArkTools.pgoAssertType(resultA, "int");

let resultB = instanceA.b + 3.2;
ArkTools.pgoAssertType(resultB, "double");
