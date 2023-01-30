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

// === tests/cases/conformance/types/objectTypeLiteral/callSignatures/parametersWithNoAnnotationAreAny.ts ===
declare function AssertType(value:any, type:string):void;
function foo(x) { 
AssertType(x, "any");
return x; 
}

let f = function foo(x) { 
AssertType(f, "(any) => any");
return x; 

AssertType(function foo(x) { return x; }, "(any) => any");

AssertType(foo, "(any) => any");

AssertType(x, "any");

AssertType(x, "any");
}

let f2 = (x) => x;
AssertType(f2, "(any) => any");
AssertType((x) => x, "(any) => any");
AssertType(x, "any");
AssertType(x, "any");

let f3 = <T>(x) => x;
AssertType(f3, "<T>(any) => any");
AssertType(<T>(x) => x, "<T>(any) => any");
AssertType(x, "any");
AssertType(x, "any");

class C {
    foo(x) {
AssertType(x, "any");
        return x;
    }
}

interface I {
    foo(x);
    foo2(x, y);
}

let a: {
AssertType(a, "{ foo(any): any; }");

    foo(x);
AssertType(foo, "(any) => any");
AssertType(x, "any");
}

let b = {
AssertType(b, "{ foo(any): any; a: (any) => any; b: (any) => any; }");
AssertType({    foo(x) {        return x;    },    a: function foo(x) {        return x;    },    b: (x) => x}, "{ foo(any): any; a: (any) => any; b: (any) => any; }");

    foo(x) {
AssertType(foo, "(any) => any");
AssertType(x, "any");

AssertType(x, "any");
        return x;

    },
    a: function foo(x) {
AssertType(a, "(any) => any");
AssertType(foo, "(any) => any");
AssertType(x, "any");
AssertType(function foo(x) {        return x;    }, "(any) => any");

AssertType(x, "any");
        return x;

    },
    b: (x) => x
AssertType(b, "(any) => any");
AssertType((x) => x, "(any) => any");
AssertType(x, "any");
AssertType(x, "any");
}

