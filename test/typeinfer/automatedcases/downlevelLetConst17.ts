/*
* Copyright (c) Microsoft Corporation. All rights reserved.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/compiler/downlevelLetConst17.ts ===
declare function AssertType(value:any, type:string):void;
'use strict'
AssertType('use strict', "string");

declare function use(a: any);

let x;
AssertType(x, "any");

for (let x = 10; ;) {
    use(x);
AssertType(use(x), "any");
AssertType(use, "(any) => any");
AssertType(x, "number");
}
use(x);
AssertType(use(x), "any");
AssertType(use, "(any) => any");
AssertType(x, "any");

for (const x = 10; ;) {
    use(x);
AssertType(use(x), "any");
AssertType(use, "(any) => any");
AssertType(x, "int");
}

for (; ;) {
    let x = 10;
AssertType(x, "number");
AssertType(10, "int");

    use(x);
AssertType(use(x), "any");
AssertType(use, "(any) => any");
AssertType(x, "number");

    x = 1;
AssertType(x = 1, "int");
AssertType(x, "number");
AssertType(1, "int");
}

for (; ;) {
    const x = 10;
AssertType(x, "int");
AssertType(10, "int");

    use(x);
AssertType(use(x), "any");
AssertType(use, "(any) => any");
AssertType(x, "int");
}

for (let x; ;) {
    use(x);
AssertType(use(x), "any");
AssertType(use, "(any) => any");
AssertType(x, "any");

    x = 1;
AssertType(x = 1, "int");
AssertType(x, "any");
AssertType(1, "int");
}

for (; ;) {
    let x;
AssertType(x, "any");

    use(x);
AssertType(use(x), "any");
AssertType(use, "(any) => any");
AssertType(x, "any");

    x = 1;
AssertType(x = 1, "int");
AssertType(x, "any");
AssertType(1, "int");
}

while (true) {
    let x;
AssertType(x, "any");

    use(x);
AssertType(use(x), "any");
AssertType(use, "(any) => any");
AssertType(x, "any");
}

while (true) {
    const x = true;
AssertType(x, "boolean");
AssertType(true, "boolean");

    use(x);
AssertType(use(x), "any");
AssertType(use, "(any) => any");
AssertType(x, "boolean");
}

do {
    let x;
AssertType(x, "any");

    use(x);
AssertType(use(x), "any");
AssertType(use, "(any) => any");
AssertType(x, "any");

} while (true);

do {
    let x;
AssertType(x, "any");

    use(x);
AssertType(use(x), "any");
AssertType(use, "(any) => any");
AssertType(x, "any");

} while (true);

for (let x in []) {
    use(x);
AssertType(use(x), "any");
AssertType(use, "(any) => any");
AssertType(x, "string");
}

for (const x in []) {
    use(x);
AssertType(use(x), "any");
AssertType(use, "(any) => any");
AssertType(x, "string");
}

for (const x of []) {
    use(x);
AssertType(use(x), "any");
AssertType(use, "(any) => any");
AssertType(x, "any");
}

