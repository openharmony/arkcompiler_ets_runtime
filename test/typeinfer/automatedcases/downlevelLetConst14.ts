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

// === tests/cases/compiler/downlevelLetConst14.ts ===
declare function AssertType(value:any, type:string):void;
'use strict'
AssertType('use strict', "string");

declare function use(a: any);

let x = 10;
AssertType(x, "number");
AssertType(10, "int");

let z0, z1, z2, z3;
AssertType(z0, "any");
AssertType(z1, "any");
AssertType(z2, "any");
AssertType(z3, "any");
{
    let x = 20;
AssertType(x, "number");
AssertType(20, "int");

    use(x);
AssertType(use(x), "any");
AssertType(use, "(any) => any");
AssertType(x, "number");

    let [z0] = [1];
AssertType(z0, "number");
AssertType([1], "[number]");
AssertType(1, "int");

    use(z0);
AssertType(use(z0), "any");
AssertType(use, "(any) => any");
AssertType(z0, "number");

    let [z1] = [1]
AssertType(z1, "number");
AssertType([1], "[number]");
AssertType(1, "int");

    use(z1);
AssertType(use(z1), "any");
AssertType(use, "(any) => any");
AssertType(z1, "number");

    let {a: z2} = { a: 1 };
AssertType(a, "any");
AssertType(z2, "number");
AssertType({ a: 1 }, "{ a: number; }");
AssertType(a, "number");
AssertType(1, "int");

    use(z2);
AssertType(use(z2), "any");
AssertType(use, "(any) => any");
AssertType(z2, "number");

    let {a: z3} = { a: 1 };
AssertType(a, "any");
AssertType(z3, "number");
AssertType({ a: 1 }, "{ a: number; }");
AssertType(a, "number");
AssertType(1, "int");

    use(z3);
AssertType(use(z3), "any");
AssertType(use, "(any) => any");
AssertType(z3, "number");
}
use(x);
AssertType(use(x), "any");
AssertType(use, "(any) => any");
AssertType(x, "number");

use(z0);
AssertType(use(z0), "any");
AssertType(use, "(any) => any");
AssertType(z0, "any");

use(z1);
AssertType(use(z1), "any");
AssertType(use, "(any) => any");
AssertType(z1, "any");

use(z2);
AssertType(use(z2), "any");
AssertType(use, "(any) => any");
AssertType(z2, "any");

use(z3);
AssertType(use(z3), "any");
AssertType(use, "(any) => any");
AssertType(z3, "any");

let z6;
AssertType(z6, "any");

let y = true;
AssertType(y, "boolean");
AssertType(true, "boolean");
{
    let y = "";
AssertType(y, "string");
AssertType("", "string");

    let [z6] = [true]
AssertType(z6, "boolean");
AssertType([true], "[boolean]");
AssertType(true, "boolean");
    {
        let y = 1;
AssertType(y, "number");
AssertType(1, "int");

        let {a: z6} = {a: 1
AssertType(a, "any");

AssertType(z6, "number");

AssertType({a: 1}, "{ a: number; }");

AssertType(a, "number");

AssertType(1, "int");
}

        use(y);
AssertType(use(y), "any");
AssertType(use, "(any) => any");
AssertType(y, "number");

        use(z6);
AssertType(use(z6), "any");
AssertType(use, "(any) => any");
AssertType(z6, "number");
    }
    use(y);
AssertType(use(y), "any");
AssertType(use, "(any) => any");
AssertType(y, "string");

    use(z6);
AssertType(use(z6), "any");
AssertType(use, "(any) => any");
AssertType(z6, "boolean");
}
use(y);
AssertType(use(y), "any");
AssertType(use, "(any) => any");
AssertType(y, "boolean");

use(z6);
AssertType(use(z6), "any");
AssertType(use, "(any) => any");
AssertType(z6, "any");

let z = false;
AssertType(z, "boolean");
AssertType(false, "boolean");

let z5 = 1;
AssertType(z5, "number");
AssertType(1, "int");
{
    let z = "";
AssertType(z, "string");
AssertType("", "string");

    let [z5] = [5];
AssertType(z5, "number");
AssertType([5], "[number]");
AssertType(5, "int");
    {
        let _z = 1;
AssertType(_z, "number");
AssertType(1, "int");

        let {a: _z5} = { a: 1 };
AssertType(a, "any");
AssertType(_z5, "number");
AssertType({ a: 1 }, "{ a: number; }");
AssertType(a, "number");
AssertType(1, "int");

        // try to step on generated name
        use(_z);
AssertType(use(_z), "any");
AssertType(use, "(any) => any");
AssertType(_z, "number");
    }
    use(z);
AssertType(use(z), "any");
AssertType(use, "(any) => any");
AssertType(z, "string");
}
use(y);
AssertType(use(y), "any");
AssertType(use, "(any) => any");
AssertType(y, "boolean");


