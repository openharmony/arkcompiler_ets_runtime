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

// === tests/cases/conformance/types/thisType/thisTypeSyntacticContext.ts ===
declare function AssertType(value:any, type:string):void;
function f(this: { n: number }) {
}

const o: { n: number, test?: (this: { n: number }) => void } = { n: 1 
AssertType(o, "{ n: number; test?: ({    n: number;}) => void; }");

AssertType(n, "number");

AssertType(test, "({    n: number;}) => void");

AssertType(this, "{ n: number; }");

AssertType(n, "number");

AssertType({ n: 1 }, "{ n: number; }");

AssertType(n, "number");

AssertType(1, "int");
}

o.test = f
AssertType(o.test = f, "({ n: number; }) => void");
AssertType(o.test, "({ n: number; }) => void");
AssertType(f, "({ n: number; }) => void");

o.test();
AssertType(o.test(), "void");
AssertType(o.test, "({ n: number; }) => void");

o!.test();
AssertType(o!.test(), "void");
AssertType(o!.test, "({ n: number; }) => void");

o.test!();
AssertType(o.test!(), "void");
AssertType(o.test!, "({ n: number; }) => void");
AssertType(o.test, "({ n: number; }) => void");

o.test!!!();
AssertType(o.test!!!(), "void");
AssertType(o.test!!!, "({ n: number; }) => void");
AssertType(o.test!!, "({ n: number; }) => void");
AssertType(o.test!, "({ n: number; }) => void");
AssertType(o.test, "({ n: number; }) => void");

(o.test!)();
AssertType((o.test!)(), "void");
AssertType((o.test!), "({ n: number; }) => void");
AssertType(o.test!, "({ n: number; }) => void");
AssertType(o.test, "({ n: number; }) => void");

(o.test)();
AssertType((o.test)(), "void");
AssertType((o.test), "({ n: number; }) => void");
AssertType(o.test, "({ n: number; }) => void");



