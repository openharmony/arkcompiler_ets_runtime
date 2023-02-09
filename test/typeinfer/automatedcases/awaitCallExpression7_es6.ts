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

// === tests/cases/conformance/async/es6/awaitCallExpression/awaitCallExpression7_es6.ts ===
declare function AssertType(value:any, type:string):void;
declare let a: boolean;
AssertType(a, "boolean");

declare let p: Promise<boolean>;
AssertType(p, "Promise<boolean>");

declare function fn(arg0: boolean, arg1: boolean, arg2: boolean): void;
declare let o: { fn(arg0: boolean, arg1: boolean, arg2: boolean): void; };
AssertType(o, "{ fn(boolean, boolean, boolean): void; }");
AssertType(fn, "(boolean, boolean, boolean) => void");
AssertType(arg0, "boolean");
AssertType(arg1, "boolean");
AssertType(arg2, "boolean");

declare let pfn: Promise<{ (arg0: boolean, arg1: boolean, arg2: boolean): void; }>;
AssertType(pfn, "Promise<(boolean, boolean, boolean) => void>");
AssertType(arg0, "boolean");
AssertType(arg1, "boolean");
AssertType(arg2, "boolean");

declare let po: Promise<{ fn(arg0: boolean, arg1: boolean, arg2: boolean): void; }>;
AssertType(po, "Promise<{ fn(boolean, boolean, boolean): void; }>");
AssertType(fn, "(boolean, boolean, boolean) => void");
AssertType(arg0, "boolean");
AssertType(arg1, "boolean");
AssertType(arg2, "boolean");

declare function before(): void;
declare function after(): void;
async function func(): Promise<void> {
    before();
AssertType(before(), "void");
AssertType(before, "() => void");

    let b = o.fn(a, await p, a);
AssertType(b, "void");
AssertType(o.fn(a, await p, a), "void");
AssertType(o.fn, "(boolean, boolean, boolean) => void");
AssertType(a, "boolean");
AssertType(await p, "boolean");
AssertType(p, "Promise<boolean>");
AssertType(a, "boolean");

    after();
AssertType(after(), "void");
AssertType(after, "() => void");
}

