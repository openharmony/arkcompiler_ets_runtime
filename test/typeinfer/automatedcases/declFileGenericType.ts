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

// === tests/cases/compiler/declFileGenericType.ts ===
declare function AssertType(value:any, type:string):void;
export module C {
    export class A<T>{ }
    export class B { }
    
    export function F<T>(x: T): A<B> { 
AssertType(null, "null");
return null; 
}

    export function F2<T>(x: T): C.A<C.B> { 
AssertType(null, "null");
return null; 
}

    export function F3<T>(x: T): C.A<C.B>[] { 
AssertType(null, "null");
return null; 
}

    export function F4<T extends A<B>>(x: T): Array<C.A<C.B>> { 
AssertType(null, "null");
return null; 
}

    export function F5<T>(): T { 
AssertType(null, "null");
return null; 
}

    export function F6<T extends A<B>>(x: T): T { 
AssertType(null, "null");
return null; 
}

    export class D<T>{

        constructor(public val: T) { }

    }
}

export let a: C.A<C.B>;
AssertType(a, "C.A<C.B>");
AssertType(C, "any");
AssertType(C, "any");

export let b = C.F;
AssertType(b, "<T>(T) => C.A<C.B>");
AssertType(C.F, "<T>(T) => C.A<C.B>");

export let c = C.F2;
AssertType(c, "<T>(T) => C.A<C.B>");
AssertType(C.F2, "<T>(T) => C.A<C.B>");

export let d = C.F3;
AssertType(d, "<T>(T) => C.A<C.B>[]");
AssertType(C.F3, "<T>(T) => C.A<C.B>[]");

export let e = C.F4;
AssertType(e, "<T extends C.A<C.B>>(T) => C.A<C.B>[]");
AssertType(C.F4, "<T extends C.A<C.B>>(T) => C.A<C.B>[]");

export let x = (new C.D<C.A<C.B>>(new C.A<C.B>())).val;
AssertType(x, "C.A<C.B>");
AssertType((new C.D<C.A<C.B>>(new C.A<C.B>())).val, "C.A<C.B>");

export function f<T extends C.A<C.B>>() { }

export let g = C.F5<C.A<C.B>>();
AssertType(g, "C.A<C.B>");
AssertType(C.F5<C.A<C.B>>(), "C.A<C.B>");
AssertType(C.F5, "<T>() => T");
AssertType(C, "any");
AssertType(C, "any");

export class h extends C.A<C.B>{ }

export interface i extends C.A<C.B> { }

export let j = C.F6;
AssertType(j, "<T extends C.A<C.B>>(T) => T");
AssertType(C.F6, "<T extends C.A<C.B>>(T) => T");


