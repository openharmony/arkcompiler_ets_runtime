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

// === tests/cases/conformance/controlFlow/controlFlowAssignmentExpression.ts ===
declare function AssertType(value:any, type:string):void;
let x: string | boolean | number;
AssertType(x, "union");

let obj: any;
AssertType(obj, "any");

x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

x = x.length;
AssertType(x = x.length, "number");
AssertType(x, "union");
AssertType(x.length, "number");

x; // number
AssertType(x, "number");

x = true;
AssertType(x = true, "boolean");
AssertType(x, "union");
AssertType(true, "boolean");

(x = "", obj).foo = (x = x.length);
AssertType((x = "", obj).foo = (x = x.length), "number");
AssertType((x = "", obj).foo, "any");
AssertType((x = x.length), "number");
AssertType(x = x.length, "number");
AssertType(x, "union");
AssertType(x.length, "number");

x; // number
AssertType(x, "number");

// https://github.com/microsoft/TypeScript/issues/35484
type D = { done: true, value: 1 } | { done: false, value: 2 };
declare function fn(): D;
let o: D;
AssertType(o, "D");

if ((o = fn()).done) {
    const y: 1 = o.value;
AssertType(y, "int");
AssertType(o.value, "int");
}

