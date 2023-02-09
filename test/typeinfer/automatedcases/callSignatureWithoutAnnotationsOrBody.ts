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

// === tests/cases/conformance/types/objectTypeLiteral/callSignatures/callSignatureWithoutAnnotationsOrBody.ts ===
declare function AssertType(value:any, type:string):void;
// Call signatures without a return type annotation and function body return 'any'

function foo(x) { }
let r = foo(1); // void since there's a body
AssertType(r, "void");
AssertType(foo(1), "void");
AssertType(foo, "(any) => void");
AssertType(1, "int");

interface I {
    ();
    f();
}
let i: I;
AssertType(i, "I");

let r2 = i();
AssertType(r2, "any");
AssertType(i(), "any");
AssertType(i, "I");

let r3 = i.f();
AssertType(r3, "any");
AssertType(i.f(), "any");
AssertType(i.f, "() => any");

let a: {
AssertType(a, "{ (): any; f(): any; }");

    ();
    f();
AssertType(f, "() => any");

};
let r4 = a();
AssertType(r4, "any");
AssertType(a(), "any");
AssertType(a, "{ (): any; f(): any; }");

let r5 = a.f();
AssertType(r5, "any");
AssertType(a.f(), "any");
AssertType(a.f, "() => any");


