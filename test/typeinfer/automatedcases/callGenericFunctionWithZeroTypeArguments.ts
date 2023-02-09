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

// === tests/cases/conformance/types/typeParameters/typeArgumentLists/callGenericFunctionWithZeroTypeArguments.ts ===
declare function AssertType(value:any, type:string):void;
// valid invocations of generic functions with no explicit type arguments provided 

function f<T>(x: T): T { 
AssertType(null, "null");
return null; 
}

let r = f(1);
AssertType(r, "number");
AssertType(f(1), "int");
AssertType(f, "<T>(T) => T");
AssertType(1, "int");

let f2 = <T>(x: T): T => { 
AssertType(f2, "<T>(T) => T");
return null; 

AssertType(<T>(x: T): T => { return null; }, "<T>(T) => T");

AssertType(x, "T");

AssertType(null, "null");
}

let r2 = f2(1);
AssertType(r2, "number");
AssertType(f2(1), "int");
AssertType(f2, "<T>(T) => T");
AssertType(1, "int");

let f3: { <T>(x: T): T; 
AssertType(f3, "<T>(T) => T");

AssertType(x, "T");
}

let r3 = f3(1);
AssertType(r3, "number");
AssertType(f3(1), "int");
AssertType(f3, "<T>(T) => T");
AssertType(1, "int");

class C {
    f<T>(x: T): T {
AssertType(null, "null");
        return null;
    }
}
let r4 = (new C()).f(1);
AssertType(r4, "number");
AssertType((new C()).f(1), "int");
AssertType((new C()).f, "<T>(T) => T");
AssertType(1, "int");

interface I {
    f<T>(x: T): T;
}
let i: I;
AssertType(i, "I");

let r5 = i.f(1);
AssertType(r5, "number");
AssertType(i.f(1), "int");
AssertType(i.f, "<T>(T) => T");
AssertType(1, "int");

class C2<T> {
    f(x: T): T {
AssertType(null, "null");
        return null;
    }
}
let r6 = (new C2()).f(1);
AssertType(r6, "unknown");
AssertType((new C2()).f(1), "unknown");
AssertType((new C2()).f, "(unknown) => unknown");
AssertType(1, "int");

interface I2<T> {
    f(x: T): T;
}
let i2: I2<number>;
AssertType(i2, "I2<number>");

let r7 = i2.f(1);
AssertType(r7, "number");
AssertType(i2.f(1), "number");
AssertType(i2.f, "(number) => number");
AssertType(1, "int");


