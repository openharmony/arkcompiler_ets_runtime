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

// === tests/cases/compiler/transformsElideNullUndefinedType.ts ===
declare function AssertType(value:any, type:string):void;
let v0: null;
AssertType(v0, "null");
AssertType(null, "null");

let v1: undefined;
AssertType(v1, "undefined");

function f0(): null { 
AssertType(null, "null");
return null; 
}

function f1(): undefined { 
AssertType(undefined, "undefined");
return undefined; 
}

let f2 = function (): null { 
AssertType(f2, "() => null");
return null; 

AssertType(function (): null { return null; }, "() => null");

AssertType(null, "null");

AssertType(null, "null");
}

let f3 = function (): undefined { 
AssertType(f3, "() => undefined");
return undefined; 

AssertType(function (): undefined { return undefined; }, "() => undefined");

AssertType(undefined, "undefined");
}

let f4 = (): null => null;
AssertType(f4, "() => null");
AssertType((): null => null, "() => null");
AssertType(null, "null");
AssertType(null, "null");

let f5 = (): undefined => undefined;
AssertType(f5, "() => undefined");
AssertType((): undefined => undefined, "() => undefined");
AssertType(undefined, "undefined");

function f6(p0: null) { }
function f7(p1: undefined) { }

let f8 = function (p2: null) { 
AssertType(f8, "(null) => void");

AssertType(function (p2: null) { }, "(null) => void");

AssertType(p2, "null");

AssertType(null, "null");
}

let f9 = function (p3: undefined) { 
AssertType(f9, "(undefined) => void");

AssertType(function (p3: undefined) { }, "(undefined) => void");

AssertType(p3, "undefined");
}

let f10 = (p4: null) => { 
AssertType(f10, "(null) => void");

AssertType((p4: null) => { }, "(null) => void");

AssertType(p4, "null");

AssertType(null, "null");
}

let f11 = (p5: undefined) => { 
AssertType(f11, "(undefined) => void");

AssertType((p5: undefined) => { }, "(undefined) => void");

AssertType(p5, "undefined");
}

class C1 {
    m0(): null { 
AssertType(null, "null");
return null; 
}

    m1(): undefined { 
AssertType(undefined, "undefined");
return undefined; 
}

    m3(p6: null) { }
    m4(p7: undefined) { }

    get a0(): null { 
AssertType(null, "null");
return null; 
}

    get a1(): undefined { 
AssertType(undefined, "undefined");
return undefined; 
}

    set a2(p8: null) { }
    set a3(p9: undefined) { }
}

class C2 { constructor(p10: null) { } }
class C3 { constructor(p11: undefined) { } }

class C4 {
    f1;
    constructor(p12: null) { }
}

class C5 {
    f2;
    constructor(p13: undefined) { }
}

let C6 = class { constructor(p12: null) { } 
AssertType(C6, "typeof C6");

AssertType(class { constructor(p12: null) { } }, "typeof C6");

AssertType(p12, "null");

AssertType(null, "null");
}

let C7 = class { constructor(p13: undefined) { } 
AssertType(C7, "typeof C7");

AssertType(class { constructor(p13: undefined) { } }, "typeof C7");

AssertType(p13, "undefined");
}

declare function fn<T>();
fn<null>();
AssertType(fn<null>(), "any");
AssertType(fn, "<T>() => any");
AssertType(null, "null");

fn<undefined>();
AssertType(fn<undefined>(), "any");
AssertType(fn, "<T>() => any");

declare class D<T> {}
new D<null>();
AssertType(new D<null>(), "D<null>");
AssertType(D, "typeof D");
AssertType(null, "null");

new D<undefined>();
AssertType(new D<undefined>(), "D<undefined>");
AssertType(D, "typeof D");


