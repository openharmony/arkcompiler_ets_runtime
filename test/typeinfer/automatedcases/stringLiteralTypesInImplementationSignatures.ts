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

// === tests/cases/conformance/types/objectTypeLiteral/callSignatures/stringLiteralTypesInImplementationSignatures.ts ===
declare function AssertType(value:any, type:string):void;
// String literal types are only valid in overload signatures

function foo(x: 'hi') { }
let f = function foo(x: 'hi') { 
AssertType(f, "('hi') => void");

AssertType(function foo(x: 'hi') { }, "('hi') => void");

AssertType(foo, "('hi') => void");

AssertType(x, "string");
}

let f2 = (x: 'hi', y: 'hi') => { 
AssertType(f2, "('hi', 'hi') => void");

AssertType((x: 'hi', y: 'hi') => { }, "('hi', 'hi') => void");

AssertType(x, "string");

AssertType(y, "string");
}

class C {
    foo(x: 'hi') { }
}

interface I {
    (x: 'hi');
    foo(x: 'hi', y: 'hi');
}

let a: {
AssertType(a, "{ ('hi'): any; foo('hi'): any; }");

    (x: 'hi');
AssertType(x, "string");

    foo(x: 'hi');
AssertType(foo, "('hi') => any");
AssertType(x, "string");
}

let b = {
AssertType(b, "{ foo('hi'): void; a: ('hi', 'hi') => void; b: ('hi') => void; }");
AssertType({    foo(x: 'hi') { },    a: function foo(x: 'hi', y: 'hi') { },    b: (x: 'hi') => { }}, "{ foo('hi'): void; a: ('hi', 'hi') => void; b: ('hi') => void; }");

    foo(x: 'hi') { },
AssertType(foo, "('hi') => void");
AssertType(x, "string");

    a: function foo(x: 'hi', y: 'hi') { },
AssertType(a, "('hi', 'hi') => void");
AssertType(function foo(x: 'hi', y: 'hi') { }, "('hi', 'hi') => void");
AssertType(foo, "('hi', 'hi') => void");
AssertType(x, "string");
AssertType(y, "string");

    b: (x: 'hi') => { 
AssertType(b, "('hi') => void");

AssertType((x: 'hi') => { }, "('hi') => void");

AssertType(x, "string");
}
}


