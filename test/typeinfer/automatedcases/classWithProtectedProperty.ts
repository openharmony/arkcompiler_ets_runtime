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

// === tests/cases/conformance/types/members/classWithProtectedProperty.ts ===
declare function AssertType(value:any, type:string):void;
// accessing any protected outside the class is an error

class C {
    protected x;
    protected a = '';
    protected b: string = '';
    protected c() { 
AssertType('', "string");
return '' 
}

    protected d = () => '';
    protected static e;
    protected static f() { 
AssertType('', "string");
return '' 
}

    protected static g = () => '';
}

class D extends C {
    method() {
        // No errors
        let d = new D();
AssertType(d, "D");
AssertType(new D(), "D");
AssertType(D, "typeof D");

        let r1: string = d.x;
AssertType(r1, "string");
AssertType(d.x, "any");

        let r2: string = d.a;
AssertType(r2, "string");
AssertType(d.a, "string");

        let r3: string = d.b;
AssertType(r3, "string");
AssertType(d.b, "string");

        let r4: string = d.c();
AssertType(r4, "string");
AssertType(d.c(), "string");
AssertType(d.c, "() => string");

        let r5: string = d.d();
AssertType(r5, "string");
AssertType(d.d(), "string");
AssertType(d.d, "() => string");

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
    }
}

