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

// === tests/cases/compiler/initializePropertiesWithRenamedLet.ts ===
declare function AssertType(value:any, type:string):void;
let x0;
AssertType(x0, "any");

if (true) {
    let x0;
AssertType(x0, "any");

    let obj1 = { x0: x0 };
AssertType(obj1, "{ x0: any; }");
AssertType({ x0: x0 }, "{ x0: any; }");
AssertType(x0, "any");
AssertType(x0, "any");

    let obj2 = { x0 };
AssertType(obj2, "{ x0: any; }");
AssertType({ x0 }, "{ x0: any; }");
AssertType(x0, "any");
}

let x, y, z;
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");

if (true) {
    let { x: x } = { x: 0 };
AssertType(x, "any");
AssertType(x, "number");
AssertType({ x: 0 }, "{ x: number; }");
AssertType(x, "number");
AssertType(0, "int");

    let { y } = { y: 0 };
AssertType(y, "number");
AssertType({ y: 0 }, "{ y: number; }");
AssertType(y, "number");
AssertType(0, "int");

    let z;
AssertType(z, "any");

    ({ z: z } = { z: 0 });
AssertType(({ z: z } = { z: 0 }), "{ z: number; }");
AssertType({ z: z } = { z: 0 }, "{ z: number; }");
AssertType({ z: z }, "{ z: any; }");
AssertType(z, "any");
AssertType(z, "any");
AssertType({ z: 0 }, "{ z: number; }");
AssertType(z, "number");
AssertType(0, "int");

    ({ z } = { z: 0 });
AssertType(({ z } = { z: 0 }), "{ z: number; }");
AssertType({ z } = { z: 0 }, "{ z: number; }");
AssertType({ z }, "{ z: any; }");
AssertType(z, "any");
AssertType({ z: 0 }, "{ z: number; }");
AssertType(z, "number");
AssertType(0, "int");
}

