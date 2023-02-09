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

// === tests/cases/conformance/expressions/binaryOperators/instanceofOperator/instanceofOperatorWithRHSIsSubtypeOfFunction.ts ===
declare function AssertType(value:any, type:string):void;
interface I extends Function { }

let x: any;
AssertType(x, "any");

let f1: Function;
AssertType(f1, "Function");

let f2: I;
AssertType(f2, "I");

let f3: { (): void };
AssertType(f3, "() => void");

let f4: { new (): number };
AssertType(f4, "new () => number");

let r1 = x instanceof f1;
AssertType(r1, "boolean");
AssertType(x instanceof f1, "boolean");
AssertType(x, "any");
AssertType(f1, "Function");

let r2 = x instanceof f2;
AssertType(r2, "boolean");
AssertType(x instanceof f2, "boolean");
AssertType(x, "any");
AssertType(f2, "I");

let r3 = x instanceof f3;
AssertType(r3, "boolean");
AssertType(x instanceof f3, "boolean");
AssertType(x, "any");
AssertType(f3, "() => void");

let r4 = x instanceof f4;
AssertType(r4, "boolean");
AssertType(x instanceof f4, "boolean");
AssertType(x, "any");
AssertType(f4, "new () => number");

let r5 = x instanceof null;
AssertType(r5, "boolean");
AssertType(x instanceof null, "boolean");
AssertType(x, "any");
AssertType(null, "null");

let r6 = x instanceof undefined;
AssertType(r6, "boolean");
AssertType(x instanceof undefined, "boolean");
AssertType(x, "any");
AssertType(undefined, "undefined");


