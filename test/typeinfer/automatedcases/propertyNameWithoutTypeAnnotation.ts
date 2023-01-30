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

// === tests/cases/conformance/types/objectTypeLiteral/propertySignatures/propertyNameWithoutTypeAnnotation.ts ===
declare function AssertType(value:any, type:string):void;
class C {
    foo;
}

interface I {
    foo;
}

let a: {
AssertType(a, "{ foo: any; }");

    foo;
AssertType(foo, "any");
}

let b = {
AssertType(b, "{ foo: any; }");
AssertType({    foo: null}, "{ foo: null; }");

    foo: null
AssertType(foo, "null");
AssertType(null, "null");
}

// These should all be of type 'any'
let r1 = (new C()).foo;
AssertType(r1, "any");
AssertType((new C()).foo, "any");

let r2 = (<I>null).foo;
AssertType(r2, "any");
AssertType((<I>null).foo, "any");

let r3 = a.foo;
AssertType(r3, "any");
AssertType(a.foo, "any");

let r4 = b.foo;
AssertType(r4, "any");
AssertType(b.foo, "any");


