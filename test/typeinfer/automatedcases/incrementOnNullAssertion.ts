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

// === tests/cases/compiler/incrementOnNullAssertion.ts ===
declare function AssertType(value:any, type:string):void;
interface Dictionary<T> {
    [myFavouriteType: string]: T | undefined
}
const x = 'bar'
AssertType(x, "string");
AssertType('bar', "string");

let foo: Dictionary<number> = {
AssertType(foo, "Dictionary<number>");

AssertType({}, "{}");
}

if (foo[x] === undefined) {
    foo[x] = 1
AssertType(foo[x] = 1, "int");
AssertType(foo[x], "union");
AssertType(foo, "Dictionary<number>");
AssertType(x, "string");
AssertType(1, "int");
}
else {
    let nu = foo[x]
AssertType(nu, "number");
AssertType(foo[x], "number");
AssertType(foo, "Dictionary<number>");
AssertType(x, "string");

    let n = foo[x]
AssertType(n, "number");
AssertType(foo[x], "number");
AssertType(foo, "Dictionary<number>");
AssertType(x, "string");

    foo[x]!++
AssertType(foo[x]!++, "number");
AssertType(foo[x]!, "number");
AssertType(foo[x], "number");
AssertType(foo, "Dictionary<number>");
AssertType(x, "string");
}


