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

// === tests/cases/conformance/types/spread/objectSpreadComputedProperty.ts ===
declare function AssertType(value:any, type:string):void;
// fixes #12200
function f() {
    let n: number = 12;
AssertType(n, "number");
AssertType(12, "int");

    let m: number = 13;
AssertType(m, "number");
AssertType(13, "int");

    let a: any = null;
AssertType(a, "any");
AssertType(null, "null");

    const o1 = { ...{}, [n]: n };
AssertType(o1, "{}");
AssertType({ ...{}, [n]: n }, "{}");
AssertType({}, "{}");
AssertType([n], "number");
AssertType(n, "number");
AssertType(n, "number");

    const o2 = { ...{}, [a]: n };
AssertType(o2, "{}");
AssertType({ ...{}, [a]: n }, "{}");
AssertType({}, "{}");
AssertType([a], "number");
AssertType(a, "any");
AssertType(n, "number");

    const o3 = { [a]: n, ...{}, [n]: n, ...{}, [m]: m };
AssertType(o3, "{}");
AssertType({ [a]: n, ...{}, [n]: n, ...{}, [m]: m }, "{}");
AssertType([a], "number");
AssertType(a, "any");
AssertType(n, "number");
AssertType({}, "{}");
AssertType([n], "number");
AssertType(n, "number");
AssertType(n, "number");
AssertType({}, "{}");
AssertType([m], "number");
AssertType(m, "number");
AssertType(m, "number");
}


