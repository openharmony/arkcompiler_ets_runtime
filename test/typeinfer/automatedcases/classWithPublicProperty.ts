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

// === tests/cases/conformance/types/members/classWithPublicProperty.ts ===
declare function AssertType(value:any, type:string):void;
class C {
    public x;
    public a = '';
    public b: string = '';
    public c() { 
AssertType('', "string");
return '' 
}

    public d = () => '';
    public static e;
    public static f() { 
AssertType('', "string");
return '' 
}

    public static g = () => '';
}

// all of these are valid
let c = new C();
AssertType(c, "C");
AssertType(new C(), "C");
AssertType(C, "typeof C");

let r1: string = c.x;
AssertType(r1, "string");
AssertType(c.x, "any");

let r2: string = c.a;
AssertType(r2, "string");
AssertType(c.a, "string");

let r3: string = c.b;
AssertType(r3, "string");
AssertType(c.b, "string");

let r4: string = c.c();
AssertType(r4, "string");
AssertType(c.c(), "string");
AssertType(c.c, "() => string");

let r5: string = c.d();
AssertType(r5, "string");
AssertType(c.d(), "string");
AssertType(c.d, "() => string");

let r6: string = C.e;
AssertType(r6, "string");
AssertType(C.e, "any");

let r7: string = C.f();
AssertType(r7, "string");
AssertType(C.f(), "string");
AssertType(C.f, "() => string");

let r8: string = C.g();
AssertType(r8, "string");
AssertType(C.g(), "string");
AssertType(C.g, "() => string");


