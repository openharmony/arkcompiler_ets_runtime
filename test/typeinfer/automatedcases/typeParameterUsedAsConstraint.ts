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

// === tests/cases/conformance/types/typeParameters/typeParameterLists/typeParameterUsedAsConstraint.ts ===
declare function AssertType(value:any, type:string):void;
class C<T, U extends T> { }
class C2<T extends U, U> { }
class C3<T extends Date, U extends T> { }
class C4<T extends U, U extends Date> { }
class C5<T extends U, U extends V, V> { }
class C6<T, U extends T, V extends U> { }

interface I<T, U extends T> { }
interface I2<T extends U, U> { }
interface I3<T extends Date, U extends T> { }
interface I4<T extends U, U extends Date> { }
interface I5<T extends U, U extends V, V> { }
interface I6<T, U extends T, V extends U> { }

function f<T, U extends T>() { }
function f2<T extends U, U>() { }
function f3<T extends Date, U extends T>() { }
function f4<T extends U, U extends Date>() { }
function f5<T extends U, U extends V, V>() { }
function f6<T, U extends T, V extends U>() { }

let e = <T, U extends T>() => { 
AssertType(e, "<T, U extends T>() => void");

AssertType(<T, U extends T>() => { }, "<T, U extends T>() => void");
}

let e2 = <T extends U, U>() => { 
AssertType(e2, "<T extends U, U>() => void");

AssertType(<T extends U, U>() => { }, "<T extends U, U>() => void");
}

let e3 = <T extends Date, U extends T>() => { 
AssertType(e3, "<T extends Date, U extends T>() => void");

AssertType(<T extends Date, U extends T>() => { }, "<T extends Date, U extends T>() => void");
}

let e4 = <T extends U, U extends Date>() => { 
AssertType(e4, "<T extends U, U extends Date>() => void");

AssertType(<T extends U, U extends Date>() => { }, "<T extends U, U extends Date>() => void");
}

let e5 = <T extends U, U extends V, V>() => { 
AssertType(e5, "<T extends U, U extends V, V>() => void");

AssertType(<T extends U, U extends V, V>() => { }, "<T extends U, U extends V, V>() => void");
}

let e6 = <T, U extends T, V extends U>() => { 
AssertType(e6, "<T, U extends T, V extends U>() => void");

AssertType(<T, U extends T, V extends U>() => { }, "<T, U extends T, V extends U>() => void");
}

let a: { <T, U extends T>(): void 
AssertType(a, "<T, U extends T>() => void");
}

let a2: { <T extends U, U>(): void 
AssertType(a2, "<T extends U, U>() => void");
}

let a3: { <T extends Date, U extends T>(): void 
AssertType(a3, "<T extends Date, U extends T>() => void");
}

let a4: { <T extends U, U extends Date>(): void 
AssertType(a4, "<T extends U, U extends Date>() => void");
}

let a5: { <T extends U, U extends V, V>(): void 
AssertType(a5, "<T extends U, U extends V, V>() => void");
}

let a6: { <T, U extends T, V extends U>(): void 
AssertType(a6, "<T, U extends T, V extends U>() => void");
}


