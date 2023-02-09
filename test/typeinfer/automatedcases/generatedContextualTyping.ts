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

// === tests/cases/conformance/expressions/contextualTyping/generatedContextualTyping.ts ===
declare function AssertType(value:any, type:string):void;
class Base { private p; }
class Derived1 extends Base { private m; }
class Derived2 extends Base { private n; }
interface Genric<T> { func(n: T[]); }
let b = new Base(), d1 = new Derived1(), d2 = new Derived2();
AssertType(b, "Base");
AssertType(new Base(), "Base");
AssertType(Base, "typeof Base");
AssertType(d1, "Derived1");
AssertType(new Derived1(), "Derived1");
AssertType(Derived1, "typeof Derived1");
AssertType(d2, "Derived2");
AssertType(new Derived2(), "Derived2");
AssertType(Derived2, "typeof Derived2");

let x1: () => Base[] = () => [d1, d2];
AssertType(x1, "() => Base[]");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x2: () => Base[] = function() { 
AssertType(x2, "() => Base[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x3: () => Base[] = function named() { 
AssertType(x3, "() => Base[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x4: { (): Base[]; } = () => [d1, d2];
AssertType(x4, "() => Base[]");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x5: { (): Base[]; } = function() { 
AssertType(x5, "() => Base[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x6: { (): Base[]; } = function named() { 
AssertType(x6, "() => Base[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x7: Base[] = [d1, d2];
AssertType(x7, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x8: Array<Base> = [d1, d2];
AssertType(x8, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x9: { [n: number]: Base; } = [d1, d2];
AssertType(x9, "{ [number]: Base; }");
AssertType(n, "number");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x10: {n: Base[]; }  = { n: [d1, d2] };
AssertType(x10, "{ n: Base[]; }");
AssertType(n, "Base[]");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x11: (s: Base[]) => any = n => { let n: Base[]; 
AssertType(x11, "(Base[]) => any");
AssertType(s, "Base[]");
AssertType(n => { let n: Base[]; return null; }, "(Base[]) => any");
AssertType(n, "Base[]");
AssertType(n, "Base[]");
AssertType(null, "null");
return null; };

let x12: Genric<Base> = { func: n => { 
AssertType(x12, "Genric<Base>");
AssertType({ func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
AssertType(func, "(Base[]) => (union)[]");
AssertType(n => { return [d1, d2]; }, "(Base[]) => (union)[]");
AssertType(n, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2]; } };

class x13 { member: () => Base[] = () => [d1, d2] }
class x14 { member: () => Base[] = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x15 { member: () => Base[] = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x16 { member: { (): Base[]; } = () => [d1, d2] }
class x17 { member: { (): Base[]; } = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x18 { member: { (): Base[]; } = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x19 { member: Base[] = [d1, d2] }
class x20 { member: Array<Base> = [d1, d2] }
class x21 { member: { [n: number]: Base; } = [d1, d2] }
class x22 { member: {n: Base[]; }  = { n: [d1, d2] } }
class x23 { member: (s: Base[]) => any = n => { let n: Base[]; 
AssertType(n, "Base[]");
return null; } 

AssertType(null, "null");
}

class x24 { member: Genric<Base> = { func: n => { 
AssertType([d1, d2], "(union)[]");
return [d1, d2]; } } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x25 { private member: () => Base[] = () => [d1, d2] }
class x26 { private member: () => Base[] = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x27 { private member: () => Base[] = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x28 { private member: { (): Base[]; } = () => [d1, d2] }
class x29 { private member: { (): Base[]; } = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x30 { private member: { (): Base[]; } = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x31 { private member: Base[] = [d1, d2] }
class x32 { private member: Array<Base> = [d1, d2] }
class x33 { private member: { [n: number]: Base; } = [d1, d2] }
class x34 { private member: {n: Base[]; }  = { n: [d1, d2] } }
class x35 { private member: (s: Base[]) => any = n => { let n: Base[]; 
AssertType(n, "Base[]");
return null; } 

AssertType(null, "null");
}

class x36 { private member: Genric<Base> = { func: n => { 
AssertType([d1, d2], "(union)[]");
return [d1, d2]; } } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x37 { public member: () => Base[] = () => [d1, d2] }
class x38 { public member: () => Base[] = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x39 { public member: () => Base[] = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x40 { public member: { (): Base[]; } = () => [d1, d2] }
class x41 { public member: { (): Base[]; } = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x42 { public member: { (): Base[]; } = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x43 { public member: Base[] = [d1, d2] }
class x44 { public member: Array<Base> = [d1, d2] }
class x45 { public member: { [n: number]: Base; } = [d1, d2] }
class x46 { public member: {n: Base[]; }  = { n: [d1, d2] } }
class x47 { public member: (s: Base[]) => any = n => { let n: Base[]; 
AssertType(n, "Base[]");
return null; } 

AssertType(null, "null");
}

class x48 { public member: Genric<Base> = { func: n => { 
AssertType([d1, d2], "(union)[]");
return [d1, d2]; } } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x49 { static member: () => Base[] = () => [d1, d2] }
class x50 { static member: () => Base[] = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x51 { static member: () => Base[] = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x52 { static member: { (): Base[]; } = () => [d1, d2] }
class x53 { static member: { (): Base[]; } = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x54 { static member: { (): Base[]; } = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x55 { static member: Base[] = [d1, d2] }
class x56 { static member: Array<Base> = [d1, d2] }
class x57 { static member: { [n: number]: Base; } = [d1, d2] }
class x58 { static member: {n: Base[]; }  = { n: [d1, d2] } }
class x59 { static member: (s: Base[]) => any = n => { let n: Base[]; 
AssertType(n, "Base[]");
return null; } 

AssertType(null, "null");
}

class x60 { static member: Genric<Base> = { func: n => { 
AssertType([d1, d2], "(union)[]");
return [d1, d2]; } } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x61 { private static member: () => Base[] = () => [d1, d2] }
class x62 { private static member: () => Base[] = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x63 { private static member: () => Base[] = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x64 { private static member: { (): Base[]; } = () => [d1, d2] }
class x65 { private static member: { (): Base[]; } = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x66 { private static member: { (): Base[]; } = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x67 { private static member: Base[] = [d1, d2] }
class x68 { private static member: Array<Base> = [d1, d2] }
class x69 { private static member: { [n: number]: Base; } = [d1, d2] }
class x70 { private static member: {n: Base[]; }  = { n: [d1, d2] } }
class x71 { private static member: (s: Base[]) => any = n => { let n: Base[]; 
AssertType(n, "Base[]");
return null; } 

AssertType(null, "null");
}

class x72 { private static member: Genric<Base> = { func: n => { 
AssertType([d1, d2], "(union)[]");
return [d1, d2]; } } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x73 { public static member: () => Base[] = () => [d1, d2] }
class x74 { public static member: () => Base[] = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x75 { public static member: () => Base[] = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x76 { public static member: { (): Base[]; } = () => [d1, d2] }
class x77 { public static member: { (): Base[]; } = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x78 { public static member: { (): Base[]; } = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x79 { public static member: Base[] = [d1, d2] }
class x80 { public static member: Array<Base> = [d1, d2] }
class x81 { public static member: { [n: number]: Base; } = [d1, d2] }
class x82 { public static member: {n: Base[]; }  = { n: [d1, d2] } }
class x83 { public static member: (s: Base[]) => any = n => { let n: Base[]; 
AssertType(n, "Base[]");
return null; } 

AssertType(null, "null");
}

class x84 { public static member: Genric<Base> = { func: n => { 
AssertType([d1, d2], "(union)[]");
return [d1, d2]; } } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x85 { constructor(parm: () => Base[] = () => [d1, d2]) { } }
class x86 { constructor(parm: () => Base[] = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }) { } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x87 { constructor(parm: () => Base[] = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }) { } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x88 { constructor(parm: { (): Base[]; } = () => [d1, d2]) { } }
class x89 { constructor(parm: { (): Base[]; } = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }) { } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x90 { constructor(parm: { (): Base[]; } = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }) { } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x91 { constructor(parm: Base[] = [d1, d2]) { } }
class x92 { constructor(parm: Array<Base> = [d1, d2]) { } }
class x93 { constructor(parm: { [n: number]: Base; } = [d1, d2]) { } }
class x94 { constructor(parm: {n: Base[]; }  = { n: [d1, d2] }) { } }
class x95 { constructor(parm: (s: Base[]) => any = n => { let n: Base[]; 
AssertType(n, "Base[]");
return null; }) { } 

AssertType(null, "null");
}

class x96 { constructor(parm: Genric<Base> = { func: n => { 
AssertType([d1, d2], "(union)[]");
return [d1, d2]; } }) { } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x97 { constructor(public parm: () => Base[] = () => [d1, d2]) { } }
class x98 { constructor(public parm: () => Base[] = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }) { } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x99 { constructor(public parm: () => Base[] = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }) { } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x100 { constructor(public parm: { (): Base[]; } = () => [d1, d2]) { } }
class x101 { constructor(public parm: { (): Base[]; } = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }) { } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x102 { constructor(public parm: { (): Base[]; } = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }) { } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x103 { constructor(public parm: Base[] = [d1, d2]) { } }
class x104 { constructor(public parm: Array<Base> = [d1, d2]) { } }
class x105 { constructor(public parm: { [n: number]: Base; } = [d1, d2]) { } }
class x106 { constructor(public parm: {n: Base[]; }  = { n: [d1, d2] }) { } }
class x107 { constructor(public parm: (s: Base[]) => any = n => { let n: Base[]; 
AssertType(n, "Base[]");
return null; }) { } 

AssertType(null, "null");
}

class x108 { constructor(public parm: Genric<Base> = { func: n => { 
AssertType([d1, d2], "(union)[]");
return [d1, d2]; } }) { } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x109 { constructor(private parm: () => Base[] = () => [d1, d2]) { } }
class x110 { constructor(private parm: () => Base[] = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }) { } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x111 { constructor(private parm: () => Base[] = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }) { } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x112 { constructor(private parm: { (): Base[]; } = () => [d1, d2]) { } }
class x113 { constructor(private parm: { (): Base[]; } = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }) { } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x114 { constructor(private parm: { (): Base[]; } = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }) { } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

class x115 { constructor(private parm: Base[] = [d1, d2]) { } }
class x116 { constructor(private parm: Array<Base> = [d1, d2]) { } }
class x117 { constructor(private parm: { [n: number]: Base; } = [d1, d2]) { } }
class x118 { constructor(private parm: {n: Base[]; }  = { n: [d1, d2] }) { } }
class x119 { constructor(private parm: (s: Base[]) => any = n => { let n: Base[]; 
AssertType(n, "Base[]");
return null; }) { } 

AssertType(null, "null");
}

class x120 { constructor(private parm: Genric<Base> = { func: n => { 
AssertType([d1, d2], "(union)[]");
return [d1, d2]; } }) { } 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x121(parm: () => Base[] = () => [d1, d2]) { }
function x122(parm: () => Base[] = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }) { 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x123(parm: () => Base[] = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }) { 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x124(parm: { (): Base[]; } = () => [d1, d2]) { }
function x125(parm: { (): Base[]; } = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }) { 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x126(parm: { (): Base[]; } = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }) { 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x127(parm: Base[] = [d1, d2]) { }
function x128(parm: Array<Base> = [d1, d2]) { }
function x129(parm: { [n: number]: Base; } = [d1, d2]) { }
function x130(parm: {n: Base[]; }  = { n: [d1, d2] }) { }
function x131(parm: (s: Base[]) => any = n => { let n: Base[]; 
AssertType(n, "Base[]");
return null; }) { 

AssertType(null, "null");
}

function x132(parm: Genric<Base> = { func: n => { 
AssertType([d1, d2], "(union)[]");
return [d1, d2]; } }) { 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x133(): () => Base[] { 
AssertType(() => [d1, d2], "() => (union)[]");
return () => [d1, d2]; 

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x134(): () => Base[] { 
AssertType(function() { return [d1, d2] }, "() => (union)[]");
return function() { return [d1, d2] }; 

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x135(): () => Base[] { 
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
return function named() { return [d1, d2] }; 

AssertType(named, "() => (union)[]");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x136(): { (): Base[]; } { 
AssertType(() => [d1, d2], "() => (union)[]");
return () => [d1, d2]; 

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x137(): { (): Base[]; } { 
AssertType(function() { return [d1, d2] }, "() => (union)[]");
return function() { return [d1, d2] }; 

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x138(): { (): Base[]; } { 
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
return function named() { return [d1, d2] }; 

AssertType(named, "() => (union)[]");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x139(): Base[] { 
AssertType([d1, d2], "(union)[]");
return [d1, d2]; 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x140(): Array<Base> { 
AssertType([d1, d2], "(union)[]");
return [d1, d2]; 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x141(): { [n: number]: Base; } { 
AssertType([d1, d2], "(union)[]");
return [d1, d2]; 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x142(): {n: Base[]; }  { 
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
return { n: [d1, d2] }; 

AssertType(n, "(union)[]");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x143(): (s: Base[]) => any { 
AssertType(n => { let n: Base[]; return null; }, "(Base[]) => any");
return n => { let n: Base[]; return null; }; 

AssertType(n, "Base[]");

AssertType(n, "Base[]");

AssertType(null, "null");
}

function x144(): Genric<Base> { 
AssertType({ func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
return { func: n => { return [d1, d2]; } }; 

AssertType(func, "(Base[]) => (union)[]");

AssertType(n => { return [d1, d2]; }, "(Base[]) => (union)[]");

AssertType(n, "Base[]");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x145(): () => Base[] { 
AssertType(() => [d1, d2], "() => (union)[]");
return () => [d1, d2]; return () => [d1, d2]; 

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");

AssertType(() => [d1, d2], "() => (union)[]");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x146(): () => Base[] { 
AssertType(function() { return [d1, d2] }, "() => (union)[]");
return function() { return [d1, d2] }; return function() { return [d1, d2] }; 

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");

AssertType(function() { return [d1, d2] }, "() => (union)[]");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x147(): () => Base[] { 
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
return function named() { return [d1, d2] }; return function named() { return [d1, d2] }; 

AssertType(named, "() => (union)[]");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");

AssertType(function named() { return [d1, d2] }, "() => (union)[]");

AssertType(named, "() => (union)[]");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x148(): { (): Base[]; } { 
AssertType(() => [d1, d2], "() => (union)[]");
return () => [d1, d2]; return () => [d1, d2]; 

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");

AssertType(() => [d1, d2], "() => (union)[]");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x149(): { (): Base[]; } { 
AssertType(function() { return [d1, d2] }, "() => (union)[]");
return function() { return [d1, d2] }; return function() { return [d1, d2] }; 

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");

AssertType(function() { return [d1, d2] }, "() => (union)[]");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x150(): { (): Base[]; } { 
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
return function named() { return [d1, d2] }; return function named() { return [d1, d2] }; 

AssertType(named, "() => (union)[]");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");

AssertType(function named() { return [d1, d2] }, "() => (union)[]");

AssertType(named, "() => (union)[]");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x151(): Base[] { 
AssertType([d1, d2], "(union)[]");
return [d1, d2]; return [d1, d2]; 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x152(): Array<Base> { 
AssertType([d1, d2], "(union)[]");
return [d1, d2]; return [d1, d2]; 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x153(): { [n: number]: Base; } { 
AssertType([d1, d2], "(union)[]");
return [d1, d2]; return [d1, d2]; 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x154(): {n: Base[]; }  { 
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
return { n: [d1, d2] }; return { n: [d1, d2] }; 

AssertType(n, "(union)[]");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");

AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");

AssertType(n, "(union)[]");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

function x155(): (s: Base[]) => any { 
AssertType(n => { let n: Base[]; return null; }, "(Base[]) => any");
return n => { let n: Base[]; return null; }; return n => { let n: Base[]; return null; }; 

AssertType(n, "Base[]");

AssertType(n, "Base[]");

AssertType(null, "null");

AssertType(n => { let n: Base[]; return null; }, "(Base[]) => any");

AssertType(n, "Base[]");

AssertType(n, "Base[]");

AssertType(null, "null");
}

function x156(): Genric<Base> { 
AssertType({ func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
return { func: n => { return [d1, d2]; } }; return { func: n => { return [d1, d2]; } }; 

AssertType(func, "(Base[]) => (union)[]");

AssertType(n => { return [d1, d2]; }, "(Base[]) => (union)[]");

AssertType(n, "Base[]");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");

AssertType({ func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");

AssertType(func, "(Base[]) => (union)[]");

AssertType(n => { return [d1, d2]; }, "(Base[]) => (union)[]");

AssertType(n, "Base[]");

AssertType([d1, d2], "(union)[]");

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

let x157: () => () => Base[] = () => { 
AssertType(x157, "() => () => Base[]");
AssertType(() => { return () => [d1, d2]; }, "() => () => (union)[]");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return () => [d1, d2]; };

let x158: () => () => Base[] = () => { 
AssertType(x158, "() => () => Base[]");
AssertType(() => { return function() { return [d1, d2] }; }, "() => () => (union)[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return function() { return [d1, d2] }; };

let x159: () => () => Base[] = () => { 
AssertType(x159, "() => () => Base[]");
AssertType(() => { return function named() { return [d1, d2] }; }, "() => () => (union)[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return function named() { return [d1, d2] }; };

let x160: () => { (): Base[]; } = () => { 
AssertType(x160, "() => {    (): Base[];}");
AssertType(() => { return () => [d1, d2]; }, "() => () => (union)[]");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return () => [d1, d2]; };

let x161: () => { (): Base[]; } = () => { 
AssertType(x161, "() => {    (): Base[];}");
AssertType(() => { return function() { return [d1, d2] }; }, "() => () => (union)[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return function() { return [d1, d2] }; };

let x162: () => { (): Base[]; } = () => { 
AssertType(x162, "() => {    (): Base[];}");
AssertType(() => { return function named() { return [d1, d2] }; }, "() => () => (union)[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return function named() { return [d1, d2] }; };

let x163: () => Base[] = () => { 
AssertType(x163, "() => Base[]");
AssertType(() => { return [d1, d2]; }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2]; };

let x164: () => Array<Base> = () => { 
AssertType(x164, "() => Array<Base>");
AssertType(() => { return [d1, d2]; }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2]; };

let x165: () => { [n: number]: Base; } = () => { 
AssertType(x165, "() => { [number]: Base; }");
AssertType(n, "number");
AssertType(() => { return [d1, d2]; }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2]; };

let x166: () => {n: Base[]; }  = () => { 
AssertType(x166, "() => {    n: Base[];}");
AssertType(n, "Base[]");
AssertType(() => { return { n: [d1, d2] }; }, "() => { n: (union)[]; }");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return { n: [d1, d2] }; };

let x167: () => (s: Base[]) => any = () => { 
AssertType(x167, "() => (Base[]) => any");
AssertType(s, "Base[]");
AssertType(() => { return n => { let n: Base[]; return null; }; }, "() => (Base[]) => any");
AssertType(n => { let n: Base[]; return null; }, "(Base[]) => any");
AssertType(n, "Base[]");
AssertType(n, "Base[]");
AssertType(null, "null");
return n => { let n: Base[]; return null; }; };

let x168: () => Genric<Base> = () => { 
AssertType(x168, "() => Genric<Base>");
AssertType(() => { return { func: n => { return [d1, d2]; } }; }, "() => { func: (Base[]) => (union)[]; }");
AssertType({ func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
AssertType(func, "(Base[]) => (union)[]");
AssertType(n => { return [d1, d2]; }, "(Base[]) => (union)[]");
AssertType(n, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return { func: n => { return [d1, d2]; } }; };

let x169: () => () => Base[] = function() { 
AssertType(x169, "() => () => Base[]");
AssertType(function() { return () => [d1, d2]; }, "() => () => (union)[]");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return () => [d1, d2]; };

let x170: () => () => Base[] = function() { 
AssertType(x170, "() => () => Base[]");
AssertType(function() { return function() { return [d1, d2] }; }, "() => () => (union)[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return function() { return [d1, d2] }; };

let x171: () => () => Base[] = function() { 
AssertType(x171, "() => () => Base[]");
AssertType(function() { return function named() { return [d1, d2] }; }, "() => () => (union)[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return function named() { return [d1, d2] }; };

let x172: () => { (): Base[]; } = function() { 
AssertType(x172, "() => {    (): Base[];}");
AssertType(function() { return () => [d1, d2]; }, "() => () => (union)[]");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return () => [d1, d2]; };

let x173: () => { (): Base[]; } = function() { 
AssertType(x173, "() => {    (): Base[];}");
AssertType(function() { return function() { return [d1, d2] }; }, "() => () => (union)[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return function() { return [d1, d2] }; };

let x174: () => { (): Base[]; } = function() { 
AssertType(x174, "() => {    (): Base[];}");
AssertType(function() { return function named() { return [d1, d2] }; }, "() => () => (union)[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return function named() { return [d1, d2] }; };

let x175: () => Base[] = function() { 
AssertType(x175, "() => Base[]");
AssertType(function() { return [d1, d2]; }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2]; };

let x176: () => Array<Base> = function() { 
AssertType(x176, "() => Array<Base>");
AssertType(function() { return [d1, d2]; }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2]; };

let x177: () => { [n: number]: Base; } = function() { 
AssertType(x177, "() => { [number]: Base; }");
AssertType(n, "number");
AssertType(function() { return [d1, d2]; }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2]; };

let x178: () => {n: Base[]; }  = function() { 
AssertType(x178, "() => {    n: Base[];}");
AssertType(n, "Base[]");
AssertType(function() { return { n: [d1, d2] }; }, "() => { n: (union)[]; }");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return { n: [d1, d2] }; };

let x179: () => (s: Base[]) => any = function() { 
AssertType(x179, "() => (Base[]) => any");
AssertType(s, "Base[]");
AssertType(function() { return n => { let n: Base[]; return null; }; }, "() => (Base[]) => any");
AssertType(n => { let n: Base[]; return null; }, "(Base[]) => any");
AssertType(n, "Base[]");
AssertType(n, "Base[]");
AssertType(null, "null");
return n => { let n: Base[]; return null; }; };

let x180: () => Genric<Base> = function() { 
AssertType(x180, "() => Genric<Base>");
AssertType(function() { return { func: n => { return [d1, d2]; } }; }, "() => { func: (Base[]) => (union)[]; }");
AssertType({ func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
AssertType(func, "(Base[]) => (union)[]");
AssertType(n => { return [d1, d2]; }, "(Base[]) => (union)[]");
AssertType(n, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return { func: n => { return [d1, d2]; } }; };

module x181 { let t: () => Base[] = () => [d1, d2]; }
module x182 { let t: () => Base[] = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }; 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

module x183 { let t: () => Base[] = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }; 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

module x184 { let t: { (): Base[]; } = () => [d1, d2]; }
module x185 { let t: { (): Base[]; } = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }; 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

module x186 { let t: { (): Base[]; } = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }; 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

module x187 { let t: Base[] = [d1, d2]; }
module x188 { let t: Array<Base> = [d1, d2]; }
module x189 { let t: { [n: number]: Base; } = [d1, d2]; }
module x190 { let t: {n: Base[]; }  = { n: [d1, d2] }; }
module x191 { let t: (s: Base[]) => any = n => { let n: Base[]; 
AssertType(n, "Base[]");
return null; }; 

AssertType(null, "null");
}

module x192 { let t: Genric<Base> = { func: n => { 
AssertType([d1, d2], "(union)[]");
return [d1, d2]; } }; 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

module x193 { export let t: () => Base[] = () => [d1, d2]; }
module x194 { export let t: () => Base[] = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }; 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

module x195 { export let t: () => Base[] = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }; 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

module x196 { export let t: { (): Base[]; } = () => [d1, d2]; }
module x197 { export let t: { (): Base[]; } = function() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }; 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

module x198 { export let t: { (): Base[]; } = function named() { 
AssertType([d1, d2], "(union)[]");
return [d1, d2] }; 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

module x199 { export let t: Base[] = [d1, d2]; }
module x200 { export let t: Array<Base> = [d1, d2]; }
module x201 { export let t: { [n: number]: Base; } = [d1, d2]; }
module x202 { export let t: {n: Base[]; }  = { n: [d1, d2] }; }
module x203 { export let t: (s: Base[]) => any = n => { let n: Base[]; 
AssertType(n, "Base[]");
return null; }; 

AssertType(null, "null");
}

module x204 { export let t: Genric<Base> = { func: n => { 
AssertType([d1, d2], "(union)[]");
return [d1, d2]; } }; 

AssertType(d1, "Derived1");

AssertType(d2, "Derived2");
}

let x206 = <() => Base[]>function() { 
AssertType(x206, "() => Base[]");
AssertType(<() => Base[]>function() { return [d1, d2] }, "() => Base[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x207 = <() => Base[]>function named() { 
AssertType(x207, "() => Base[]");
AssertType(<() => Base[]>function named() { return [d1, d2] }, "() => Base[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x209 = <{ (): Base[]; }>function() { 
AssertType(x209, "() => Base[]");
AssertType(<{ (): Base[]; }>function() { return [d1, d2] }, "() => Base[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x210 = <{ (): Base[]; }>function named() { 
AssertType(x210, "() => Base[]");
AssertType(<{ (): Base[]; }>function named() { return [d1, d2] }, "() => Base[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x211 = <Base[]>[d1, d2];
AssertType(x211, "Base[]");
AssertType(<Base[]>[d1, d2], "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x212 = <Array<Base>>[d1, d2];
AssertType(x212, "Base[]");
AssertType(<Array<Base>>[d1, d2], "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x213 = <{ [n: number]: Base; }>[d1, d2];
AssertType(x213, "{ [number]: Base; }");
AssertType(<{ [n: number]: Base; }>[d1, d2], "{ [number]: Base; }");
AssertType(n, "number");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x214 = <{n: Base[]; } >{ n: [d1, d2] };
AssertType(x214, "{ n: Base[]; }");
AssertType(<{n: Base[]; } >{ n: [d1, d2] }, "{ n: Base[]; }");
AssertType(n, "Base[]");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x216 = <Genric<Base>>{ func: n => { 
AssertType(x216, "Genric<Base>");
AssertType(<Genric<Base>>{ func: n => { return [d1, d2]; } }, "Genric<Base>");
AssertType({ func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
AssertType(func, "(Base[]) => (union)[]");
AssertType(n => { return [d1, d2]; }, "(Base[]) => (union)[]");
AssertType(n, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2]; } };

let x217 = (<() => Base[]>undefined) || function() { 
AssertType(x217, "() => Base[]");
AssertType((<() => Base[]>undefined) || function() { return [d1, d2] }, "() => Base[]");
AssertType((<() => Base[]>undefined), "() => Base[]");
AssertType(<() => Base[]>undefined, "() => Base[]");
AssertType(undefined, "undefined");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x218 = (<() => Base[]>undefined) || function named() { 
AssertType(x218, "() => Base[]");
AssertType((<() => Base[]>undefined) || function named() { return [d1, d2] }, "() => Base[]");
AssertType((<() => Base[]>undefined), "() => Base[]");
AssertType(<() => Base[]>undefined, "() => Base[]");
AssertType(undefined, "undefined");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x219 = (<{ (): Base[]; }>undefined) || function() { 
AssertType(x219, "() => Base[]");
AssertType((<{ (): Base[]; }>undefined) || function() { return [d1, d2] }, "() => Base[]");
AssertType((<{ (): Base[]; }>undefined), "() => Base[]");
AssertType(<{ (): Base[]; }>undefined, "() => Base[]");
AssertType(undefined, "undefined");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x220 = (<{ (): Base[]; }>undefined) || function named() { 
AssertType(x220, "() => Base[]");
AssertType((<{ (): Base[]; }>undefined) || function named() { return [d1, d2] }, "() => Base[]");
AssertType((<{ (): Base[]; }>undefined), "() => Base[]");
AssertType(<{ (): Base[]; }>undefined, "() => Base[]");
AssertType(undefined, "undefined");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x221 = (<Base[]>undefined) || [d1, d2];
AssertType(x221, "Base[]");
AssertType((<Base[]>undefined) || [d1, d2], "Base[]");
AssertType((<Base[]>undefined), "Base[]");
AssertType(<Base[]>undefined, "Base[]");
AssertType(undefined, "undefined");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x222 = (<Array<Base>>undefined) || [d1, d2];
AssertType(x222, "Base[]");
AssertType((<Array<Base>>undefined) || [d1, d2], "Base[]");
AssertType((<Array<Base>>undefined), "Base[]");
AssertType(<Array<Base>>undefined, "Base[]");
AssertType(undefined, "undefined");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x223 = (<{ [n: number]: Base; }>undefined) || [d1, d2];
AssertType(x223, "{ [number]: Base; }");
AssertType((<{ [n: number]: Base; }>undefined) || [d1, d2], "{ [number]: Base; }");
AssertType((<{ [n: number]: Base; }>undefined), "{ [number]: Base; }");
AssertType(<{ [n: number]: Base; }>undefined, "{ [number]: Base; }");
AssertType(n, "number");
AssertType(undefined, "undefined");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x224 = (<{n: Base[]; } >undefined) || { n: [d1, d2] };
AssertType(x224, "{ n: Base[]; }");
AssertType((<{n: Base[]; } >undefined) || { n: [d1, d2] }, "{ n: Base[]; }");
AssertType((<{n: Base[]; } >undefined), "{ n: Base[]; }");
AssertType(<{n: Base[]; } >undefined, "{ n: Base[]; }");
AssertType(n, "Base[]");
AssertType(undefined, "undefined");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x225: () => Base[]; x225 = () => [d1, d2];
AssertType(x225, "() => Base[]");
AssertType(x225 = () => [d1, d2], "() => (union)[]");
AssertType(x225, "() => Base[]");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x226: () => Base[]; x226 = function() { 
AssertType(x226, "() => Base[]");
AssertType(x226 = function() { return [d1, d2] }, "() => (union)[]");
AssertType(x226, "() => Base[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x227: () => Base[]; x227 = function named() { 
AssertType(x227, "() => Base[]");
AssertType(x227 = function named() { return [d1, d2] }, "() => (union)[]");
AssertType(x227, "() => Base[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x228: { (): Base[]; }; x228 = () => [d1, d2];
AssertType(x228, "() => Base[]");
AssertType(x228 = () => [d1, d2], "() => (union)[]");
AssertType(x228, "() => Base[]");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x229: { (): Base[]; }; x229 = function() { 
AssertType(x229, "() => Base[]");
AssertType(x229 = function() { return [d1, d2] }, "() => (union)[]");
AssertType(x229, "() => Base[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x230: { (): Base[]; }; x230 = function named() { 
AssertType(x230, "() => Base[]");
AssertType(x230 = function named() { return [d1, d2] }, "() => (union)[]");
AssertType(x230, "() => Base[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x231: Base[]; x231 = [d1, d2];
AssertType(x231, "Base[]");
AssertType(x231 = [d1, d2], "(union)[]");
AssertType(x231, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x232: Array<Base>; x232 = [d1, d2];
AssertType(x232, "Base[]");
AssertType(x232 = [d1, d2], "(union)[]");
AssertType(x232, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x233: { [n: number]: Base; }; x233 = [d1, d2];
AssertType(x233, "{ [number]: Base; }");
AssertType(n, "number");
AssertType(x233 = [d1, d2], "(union)[]");
AssertType(x233, "{ [number]: Base; }");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x234: {n: Base[]; } ; x234 = { n: [d1, d2] };
AssertType(x234, "{ n: Base[]; }");
AssertType(n, "Base[]");
AssertType(x234 = { n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(x234, "{ n: Base[]; }");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x235: (s: Base[]) => any; x235 = n => { let n: Base[]; 
AssertType(x235, "(Base[]) => any");
AssertType(s, "Base[]");
AssertType(x235 = n => { let n: Base[]; return null; }, "(Base[]) => any");
AssertType(x235, "(Base[]) => any");
AssertType(n => { let n: Base[]; return null; }, "(Base[]) => any");
AssertType(n, "Base[]");
AssertType(n, "Base[]");
AssertType(null, "null");
return null; };

let x236: Genric<Base>; x236 = { func: n => { 
AssertType(x236, "Genric<Base>");
AssertType(x236 = { func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
AssertType(x236, "Genric<Base>");
AssertType({ func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
AssertType(func, "(Base[]) => (union)[]");
AssertType(n => { return [d1, d2]; }, "(Base[]) => (union)[]");
AssertType(n, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2]; } };

let x237: { n: () => Base[]; } = { n: () => [d1, d2] };
AssertType(x237, "{ n: () => Base[]; }");
AssertType(n, "() => Base[]");
AssertType({ n: () => [d1, d2] }, "{ n: () => (union)[]; }");
AssertType(n, "() => (union)[]");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x238: { n: () => Base[]; } = { n: function() { 
AssertType(x238, "{ n: () => Base[]; }");
AssertType(n, "() => Base[]");
AssertType({ n: function() { return [d1, d2] } }, "{ n: () => (union)[]; }");
AssertType(n, "() => (union)[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] } };

let x239: { n: () => Base[]; } = { n: function named() { 
AssertType(x239, "{ n: () => Base[]; }");
AssertType(n, "() => Base[]");
AssertType({ n: function named() { return [d1, d2] } }, "{ n: () => (union)[]; }");
AssertType(n, "() => (union)[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] } };

let x240: { n: { (): Base[]; }; } = { n: () => [d1, d2] };
AssertType(x240, "{ n: {    (): Base[];}; }");
AssertType(n, "() => Base[]");
AssertType({ n: () => [d1, d2] }, "{ n: () => (union)[]; }");
AssertType(n, "() => (union)[]");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x241: { n: { (): Base[]; }; } = { n: function() { 
AssertType(x241, "{ n: {    (): Base[];}; }");
AssertType(n, "() => Base[]");
AssertType({ n: function() { return [d1, d2] } }, "{ n: () => (union)[]; }");
AssertType(n, "() => (union)[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] } };

let x242: { n: { (): Base[]; }; } = { n: function named() { 
AssertType(x242, "{ n: {    (): Base[];}; }");
AssertType(n, "() => Base[]");
AssertType({ n: function named() { return [d1, d2] } }, "{ n: () => (union)[]; }");
AssertType(n, "() => (union)[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] } };

let x243: { n: Base[]; } = { n: [d1, d2] };
AssertType(x243, "{ n: Base[]; }");
AssertType(n, "Base[]");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x244: { n: Array<Base>; } = { n: [d1, d2] };
AssertType(x244, "{ n: Array<Base>; }");
AssertType(n, "Base[]");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x245: { n: { [n: number]: Base; }; } = { n: [d1, d2] };
AssertType(x245, "{ n: { [number]: Base; }; }");
AssertType(n, "{ [number]: Base; }");
AssertType(n, "number");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x246: { n: {n: Base[]; } ; } = { n: { n: [d1, d2] } };
AssertType(x246, "{ n: {    n: Base[];}; }");
AssertType(n, "{ n: Base[]; }");
AssertType(n, "Base[]");
AssertType({ n: { n: [d1, d2] } }, "{ n: { n: (union)[]; }; }");
AssertType(n, "{ n: (union)[]; }");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x247: { n: (s: Base[]) => any; } = { n: n => { let n: Base[]; 
AssertType(x247, "{ n: (Base[]) => any; }");
AssertType(n, "(Base[]) => any");
AssertType(s, "Base[]");
AssertType({ n: n => { let n: Base[]; return null; } }, "{ n: (Base[]) => any; }");
AssertType(n, "(Base[]) => any");
AssertType(n => { let n: Base[]; return null; }, "(Base[]) => any");
AssertType(n, "Base[]");
AssertType(n, "Base[]");
AssertType(null, "null");
return null; } };

let x248: { n: Genric<Base>; } = { n: { func: n => { 
AssertType(x248, "{ n: Genric<Base>; }");
AssertType(n, "Genric<Base>");
AssertType({ n: { func: n => { return [d1, d2]; } } }, "{ n: { func: (Base[]) => (union)[]; }; }");
AssertType(n, "{ func: (Base[]) => (union)[]; }");
AssertType({ func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
AssertType(func, "(Base[]) => (union)[]");
AssertType(n => { return [d1, d2]; }, "(Base[]) => (union)[]");
AssertType(n, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2]; } } };

let x252: { (): Base[]; }[] = [() => [d1, d2]];
AssertType(x252, "(() => Base[])[]");
AssertType([() => [d1, d2]], "(() => (union)[])[]");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x253: { (): Base[]; }[] = [function() { 
AssertType(x253, "(() => Base[])[]");
AssertType([function() { return [d1, d2] }], "(() => (union)[])[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] }];

let x254: { (): Base[]; }[] = [function named() { 
AssertType(x254, "(() => Base[])[]");
AssertType([function named() { return [d1, d2] }], "(() => (union)[])[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] }];

let x255: Base[][] = [[d1, d2]];
AssertType(x255, "Base[][]");
AssertType([[d1, d2]], "(union)[][]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x256: Array<Base>[] = [[d1, d2]];
AssertType(x256, "Base[][]");
AssertType([[d1, d2]], "(union)[][]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x257: { [n: number]: Base; }[] = [[d1, d2]];
AssertType(x257, "{ [number]: Base; }[]");
AssertType(n, "number");
AssertType([[d1, d2]], "(union)[][]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x258: {n: Base[]; } [] = [{ n: [d1, d2] }];
AssertType(x258, "{ n: Base[]; }[]");
AssertType(n, "Base[]");
AssertType([{ n: [d1, d2] }], "{ n: (union)[]; }[]");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x260: Genric<Base>[] = [{ func: n => { 
AssertType(x260, "Genric<Base>[]");
AssertType([{ func: n => { return [d1, d2]; } }], "{ func: (Base[]) => (union)[]; }[]");
AssertType({ func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
AssertType(func, "(Base[]) => (union)[]");
AssertType(n => { return [d1, d2]; }, "(Base[]) => (union)[]");
AssertType(n, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2]; } }];

let x261: () => Base[] = function() { 
AssertType(x261, "() => Base[]");
AssertType(function() { return [d1, d2] } || undefined, "() => (union)[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");
return [d1, d2] } || undefined;

let x262: () => Base[] = function named() { 
AssertType(x262, "() => Base[]");
AssertType(function named() { return [d1, d2] } || undefined, "() => (union)[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");
return [d1, d2] } || undefined;

let x263: { (): Base[]; } = function() { 
AssertType(x263, "() => Base[]");
AssertType(function() { return [d1, d2] } || undefined, "() => (union)[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");
return [d1, d2] } || undefined;

let x264: { (): Base[]; } = function named() { 
AssertType(x264, "() => Base[]");
AssertType(function named() { return [d1, d2] } || undefined, "() => (union)[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");
return [d1, d2] } || undefined;

let x265: Base[] = [d1, d2] || undefined;
AssertType(x265, "Base[]");
AssertType([d1, d2] || undefined, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");

let x266: Array<Base> = [d1, d2] || undefined;
AssertType(x266, "Base[]");
AssertType([d1, d2] || undefined, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");

let x267: { [n: number]: Base; } = [d1, d2] || undefined;
AssertType(x267, "{ [number]: Base; }");
AssertType(n, "number");
AssertType([d1, d2] || undefined, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");

let x268: {n: Base[]; }  = { n: [d1, d2] } || undefined;
AssertType(x268, "{ n: Base[]; }");
AssertType(n, "Base[]");
AssertType({ n: [d1, d2] } || undefined, "{ n: (union)[]; }");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");

let x269: () => Base[] = undefined || function() { 
AssertType(x269, "() => Base[]");
AssertType(undefined || function() { return [d1, d2] }, "() => (union)[]");
AssertType(undefined, "undefined");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x270: () => Base[] = undefined || function named() { 
AssertType(x270, "() => Base[]");
AssertType(undefined || function named() { return [d1, d2] }, "() => (union)[]");
AssertType(undefined, "undefined");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x271: { (): Base[]; } = undefined || function() { 
AssertType(x271, "() => Base[]");
AssertType(undefined || function() { return [d1, d2] }, "() => (union)[]");
AssertType(undefined, "undefined");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x272: { (): Base[]; } = undefined || function named() { 
AssertType(x272, "() => Base[]");
AssertType(undefined || function named() { return [d1, d2] }, "() => (union)[]");
AssertType(undefined, "undefined");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x273: Base[] = undefined || [d1, d2];
AssertType(x273, "Base[]");
AssertType(undefined || [d1, d2], "(union)[]");
AssertType(undefined, "undefined");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x274: Array<Base> = undefined || [d1, d2];
AssertType(x274, "Base[]");
AssertType(undefined || [d1, d2], "(union)[]");
AssertType(undefined, "undefined");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x275: { [n: number]: Base; } = undefined || [d1, d2];
AssertType(x275, "{ [number]: Base; }");
AssertType(n, "number");
AssertType(undefined || [d1, d2], "(union)[]");
AssertType(undefined, "undefined");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x276: {n: Base[]; }  = undefined || { n: [d1, d2] };
AssertType(x276, "{ n: Base[]; }");
AssertType(n, "Base[]");
AssertType(undefined || { n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(undefined, "undefined");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x277: () => Base[] = function() { 
AssertType(x277, "() => Base[]");
AssertType(function() { return [d1, d2] } || function() { return [d1, d2] }, "() => (union)[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] } || function() { return [d1, d2] };

let x278: () => Base[] = function named() { 
AssertType(x278, "() => Base[]");
AssertType(function named() { return [d1, d2] } || function named() { return [d1, d2] }, "() => (union)[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] } || function named() { return [d1, d2] };

let x279: { (): Base[]; } = function() { 
AssertType(x279, "() => Base[]");
AssertType(function() { return [d1, d2] } || function() { return [d1, d2] }, "() => (union)[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] } || function() { return [d1, d2] };

let x280: { (): Base[]; } = function named() { 
AssertType(x280, "() => Base[]");
AssertType(function named() { return [d1, d2] } || function named() { return [d1, d2] }, "() => (union)[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] } || function named() { return [d1, d2] };

let x281: Base[] = [d1, d2] || [d1, d2];
AssertType(x281, "Base[]");
AssertType([d1, d2] || [d1, d2], "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x282: Array<Base> = [d1, d2] || [d1, d2];
AssertType(x282, "Base[]");
AssertType([d1, d2] || [d1, d2], "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x283: { [n: number]: Base; } = [d1, d2] || [d1, d2];
AssertType(x283, "{ [number]: Base; }");
AssertType(n, "number");
AssertType([d1, d2] || [d1, d2], "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x284: {n: Base[]; }  = { n: [d1, d2] } || { n: [d1, d2] };
AssertType(x284, "{ n: Base[]; }");
AssertType(n, "Base[]");
AssertType({ n: [d1, d2] } || { n: [d1, d2] }, "{ n: (union)[]; }");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x285: () => Base[] = true ? () => [d1, d2] : () => [d1, d2];
AssertType(x285, "() => Base[]");
AssertType(true ? () => [d1, d2] : () => [d1, d2], "() => (union)[]");
AssertType(true, "boolean");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x286: () => Base[] = true ? function() { 
AssertType(x286, "() => Base[]");
AssertType(true ? function() { return [d1, d2] } : function() { return [d1, d2] }, "() => (union)[]");
AssertType(true, "boolean");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] } : function() { return [d1, d2] };

let x287: () => Base[] = true ? function named() { 
AssertType(x287, "() => Base[]");
AssertType(true ? function named() { return [d1, d2] } : function named() { return [d1, d2] }, "() => (union)[]");
AssertType(true, "boolean");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] } : function named() { return [d1, d2] };

let x288: { (): Base[]; } = true ? () => [d1, d2] : () => [d1, d2];
AssertType(x288, "() => Base[]");
AssertType(true ? () => [d1, d2] : () => [d1, d2], "() => (union)[]");
AssertType(true, "boolean");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x289: { (): Base[]; } = true ? function() { 
AssertType(x289, "() => Base[]");
AssertType(true ? function() { return [d1, d2] } : function() { return [d1, d2] }, "() => (union)[]");
AssertType(true, "boolean");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] } : function() { return [d1, d2] };

let x290: { (): Base[]; } = true ? function named() { 
AssertType(x290, "() => Base[]");
AssertType(true ? function named() { return [d1, d2] } : function named() { return [d1, d2] }, "() => (union)[]");
AssertType(true, "boolean");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] } : function named() { return [d1, d2] };

let x291: Base[] = true ? [d1, d2] : [d1, d2];
AssertType(x291, "Base[]");
AssertType(true ? [d1, d2] : [d1, d2], "(union)[]");
AssertType(true, "boolean");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x292: Array<Base> = true ? [d1, d2] : [d1, d2];
AssertType(x292, "Base[]");
AssertType(true ? [d1, d2] : [d1, d2], "(union)[]");
AssertType(true, "boolean");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x293: { [n: number]: Base; } = true ? [d1, d2] : [d1, d2];
AssertType(x293, "{ [number]: Base; }");
AssertType(n, "number");
AssertType(true ? [d1, d2] : [d1, d2], "(union)[]");
AssertType(true, "boolean");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x294: {n: Base[]; }  = true ? { n: [d1, d2] } : { n: [d1, d2] };
AssertType(x294, "{ n: Base[]; }");
AssertType(n, "Base[]");
AssertType(true ? { n: [d1, d2] } : { n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(true, "boolean");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x295: (s: Base[]) => any = true ? n => { let n: Base[]; 
AssertType(x295, "(Base[]) => any");
AssertType(s, "Base[]");
AssertType(true ? n => { let n: Base[]; return null; } : n => { let n: Base[]; return null; }, "(Base[]) => any");
AssertType(true, "boolean");
AssertType(n => { let n: Base[]; return null; }, "(Base[]) => any");
AssertType(n, "Base[]");
AssertType(n, "Base[]");
AssertType(null, "null");
AssertType(n => { let n: Base[]; return null; }, "(Base[]) => any");
AssertType(n, "Base[]");
AssertType(n, "Base[]");
AssertType(null, "null");
return null; } : n => { let n: Base[]; return null; };

let x296: Genric<Base> = true ? { func: n => { 
AssertType(x296, "Genric<Base>");
AssertType(true ? { func: n => { return [d1, d2]; } } : { func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
AssertType(true, "boolean");
AssertType({ func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
AssertType(func, "(Base[]) => (union)[]");
AssertType(n => { return [d1, d2]; }, "(Base[]) => (union)[]");
AssertType(n, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType({ func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
AssertType(func, "(Base[]) => (union)[]");
AssertType(n => { return [d1, d2]; }, "(Base[]) => (union)[]");
AssertType(n, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2]; } } : { func: n => { return [d1, d2]; } };

let x297: () => Base[] = true ? undefined : () => [d1, d2];
AssertType(x297, "() => Base[]");
AssertType(true ? undefined : () => [d1, d2], "() => (union)[]");
AssertType(true, "boolean");
AssertType(undefined, "undefined");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x298: () => Base[] = true ? undefined : function() { 
AssertType(x298, "() => Base[]");
AssertType(true ? undefined : function() { return [d1, d2] }, "() => (union)[]");
AssertType(true, "boolean");
AssertType(undefined, "undefined");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x299: () => Base[] = true ? undefined : function named() { 
AssertType(x299, "() => Base[]");
AssertType(true ? undefined : function named() { return [d1, d2] }, "() => (union)[]");
AssertType(true, "boolean");
AssertType(undefined, "undefined");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x300: { (): Base[]; } = true ? undefined : () => [d1, d2];
AssertType(x300, "() => Base[]");
AssertType(true ? undefined : () => [d1, d2], "() => (union)[]");
AssertType(true, "boolean");
AssertType(undefined, "undefined");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x301: { (): Base[]; } = true ? undefined : function() { 
AssertType(x301, "() => Base[]");
AssertType(true ? undefined : function() { return [d1, d2] }, "() => (union)[]");
AssertType(true, "boolean");
AssertType(undefined, "undefined");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x302: { (): Base[]; } = true ? undefined : function named() { 
AssertType(x302, "() => Base[]");
AssertType(true ? undefined : function named() { return [d1, d2] }, "() => (union)[]");
AssertType(true, "boolean");
AssertType(undefined, "undefined");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] };

let x303: Base[] = true ? undefined : [d1, d2];
AssertType(x303, "Base[]");
AssertType(true ? undefined : [d1, d2], "(union)[]");
AssertType(true, "boolean");
AssertType(undefined, "undefined");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x304: Array<Base> = true ? undefined : [d1, d2];
AssertType(x304, "Base[]");
AssertType(true ? undefined : [d1, d2], "(union)[]");
AssertType(true, "boolean");
AssertType(undefined, "undefined");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x305: { [n: number]: Base; } = true ? undefined : [d1, d2];
AssertType(x305, "{ [number]: Base; }");
AssertType(n, "number");
AssertType(true ? undefined : [d1, d2], "(union)[]");
AssertType(true, "boolean");
AssertType(undefined, "undefined");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x306: {n: Base[]; }  = true ? undefined : { n: [d1, d2] };
AssertType(x306, "{ n: Base[]; }");
AssertType(n, "Base[]");
AssertType(true ? undefined : { n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(true, "boolean");
AssertType(undefined, "undefined");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x307: (s: Base[]) => any = true ? undefined : n => { let n: Base[]; 
AssertType(x307, "(Base[]) => any");
AssertType(s, "Base[]");
AssertType(true ? undefined : n => { let n: Base[]; return null; }, "(Base[]) => any");
AssertType(true, "boolean");
AssertType(undefined, "undefined");
AssertType(n => { let n: Base[]; return null; }, "(Base[]) => any");
AssertType(n, "Base[]");
AssertType(n, "Base[]");
AssertType(null, "null");
return null; };

let x308: Genric<Base> = true ? undefined : { func: n => { 
AssertType(x308, "Genric<Base>");
AssertType(true ? undefined : { func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
AssertType(true, "boolean");
AssertType(undefined, "undefined");
AssertType({ func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
AssertType(func, "(Base[]) => (union)[]");
AssertType(n => { return [d1, d2]; }, "(Base[]) => (union)[]");
AssertType(n, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2]; } };

let x309: () => Base[] = true ? () => [d1, d2] : undefined;
AssertType(x309, "() => Base[]");
AssertType(true ? () => [d1, d2] : undefined, "() => (union)[]");
AssertType(true, "boolean");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");

let x310: () => Base[] = true ? function() { 
AssertType(x310, "() => Base[]");
AssertType(true ? function() { return [d1, d2] } : undefined, "() => (union)[]");
AssertType(true, "boolean");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");
return [d1, d2] } : undefined;

let x311: () => Base[] = true ? function named() { 
AssertType(x311, "() => Base[]");
AssertType(true ? function named() { return [d1, d2] } : undefined, "() => (union)[]");
AssertType(true, "boolean");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");
return [d1, d2] } : undefined;

let x312: { (): Base[]; } = true ? () => [d1, d2] : undefined;
AssertType(x312, "() => Base[]");
AssertType(true ? () => [d1, d2] : undefined, "() => (union)[]");
AssertType(true, "boolean");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");

let x313: { (): Base[]; } = true ? function() { 
AssertType(x313, "() => Base[]");
AssertType(true ? function() { return [d1, d2] } : undefined, "() => (union)[]");
AssertType(true, "boolean");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");
return [d1, d2] } : undefined;

let x314: { (): Base[]; } = true ? function named() { 
AssertType(x314, "() => Base[]");
AssertType(true ? function named() { return [d1, d2] } : undefined, "() => (union)[]");
AssertType(true, "boolean");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");
return [d1, d2] } : undefined;

let x315: Base[] = true ? [d1, d2] : undefined;
AssertType(x315, "Base[]");
AssertType(true ? [d1, d2] : undefined, "(union)[]");
AssertType(true, "boolean");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");

let x316: Array<Base> = true ? [d1, d2] : undefined;
AssertType(x316, "Base[]");
AssertType(true ? [d1, d2] : undefined, "(union)[]");
AssertType(true, "boolean");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");

let x317: { [n: number]: Base; } = true ? [d1, d2] : undefined;
AssertType(x317, "{ [number]: Base; }");
AssertType(n, "number");
AssertType(true ? [d1, d2] : undefined, "(union)[]");
AssertType(true, "boolean");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");

let x318: {n: Base[]; }  = true ? { n: [d1, d2] } : undefined;
AssertType(x318, "{ n: Base[]; }");
AssertType(n, "Base[]");
AssertType(true ? { n: [d1, d2] } : undefined, "{ n: (union)[]; }");
AssertType(true, "boolean");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");

let x319: (s: Base[]) => any = true ? n => { let n: Base[]; 
AssertType(x319, "(Base[]) => any");
AssertType(s, "Base[]");
AssertType(true ? n => { let n: Base[]; return null; } : undefined, "(Base[]) => any");
AssertType(true, "boolean");
AssertType(n => { let n: Base[]; return null; }, "(Base[]) => any");
AssertType(n, "Base[]");
AssertType(n, "Base[]");
AssertType(null, "null");
AssertType(undefined, "undefined");
return null; } : undefined;

let x320: Genric<Base> = true ? { func: n => { 
AssertType(x320, "Genric<Base>");
AssertType(true ? { func: n => { return [d1, d2]; } } : undefined, "{ func: (Base[]) => (union)[]; }");
AssertType(true, "boolean");
AssertType({ func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
AssertType(func, "(Base[]) => (union)[]");
AssertType(n => { return [d1, d2]; }, "(Base[]) => (union)[]");
AssertType(n, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
AssertType(undefined, "undefined");
return [d1, d2]; } } : undefined;

function x321(n: () => Base[]) { }; x321(() => [d1, d2]);
AssertType(x321(() => [d1, d2]), "void");
AssertType(x321, "(() => Base[]) => void");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

function x322(n: () => Base[]) { }; x322(function() { 
AssertType(x322(function() { return [d1, d2] }), "void");
AssertType(x322, "(() => Base[]) => void");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] });

function x323(n: () => Base[]) { }; x323(function named() { 
AssertType(x323(function named() { return [d1, d2] }), "void");
AssertType(x323, "(() => Base[]) => void");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] });

function x324(n: { (): Base[]; }) { }; x324(() => [d1, d2]);
AssertType(x324(() => [d1, d2]), "void");
AssertType(x324, "(() => Base[]) => void");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

function x325(n: { (): Base[]; }) { }; x325(function() { 
AssertType(x325(function() { return [d1, d2] }), "void");
AssertType(x325, "(() => Base[]) => void");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] });

function x326(n: { (): Base[]; }) { }; x326(function named() { 
AssertType(x326(function named() { return [d1, d2] }), "void");
AssertType(x326, "(() => Base[]) => void");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] });

function x327(n: Base[]) { }; x327([d1, d2]);
AssertType(x327([d1, d2]), "void");
AssertType(x327, "(Base[]) => void");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

function x328(n: Array<Base>) { }; x328([d1, d2]);
AssertType(x328([d1, d2]), "void");
AssertType(x328, "(Base[]) => void");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

function x329(n: { [n: number]: Base; }) { }; x329([d1, d2]);
AssertType(x329([d1, d2]), "void");
AssertType(x329, "({ [number]: Base; }) => void");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

function x330(n: {n: Base[]; } ) { }; x330({ n: [d1, d2] });
AssertType(x330({ n: [d1, d2] }), "void");
AssertType(x330, "({ n: Base[]; }) => void");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

function x331(n: (s: Base[]) => any) { }; x331(n => { let n: Base[]; 
AssertType(x331(n => { let n: Base[]; return null; }), "void");
AssertType(x331, "((Base[]) => any) => void");
AssertType(n => { let n: Base[]; return null; }, "(Base[]) => any");
AssertType(n, "Base[]");
AssertType(n, "Base[]");
AssertType(null, "null");
return null; });

function x332(n: Genric<Base>) { }; x332({ func: n => { 
AssertType(x332({ func: n => { return [d1, d2]; } }), "void");
AssertType(x332, "(Genric<Base>) => void");
AssertType({ func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
AssertType(func, "(Base[]) => (union)[]");
AssertType(n => { return [d1, d2]; }, "(Base[]) => (union)[]");
AssertType(n, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2]; } });

let x333 = (n: () => Base[]) => n; x333(() => [d1, d2]);
AssertType(x333, "(() => Base[]) => () => Base[]");
AssertType((n: () => Base[]) => n, "(() => Base[]) => () => Base[]");
AssertType(n, "() => Base[]");
AssertType(n, "() => Base[]");
AssertType(x333(() => [d1, d2]), "() => Base[]");
AssertType(x333, "(() => Base[]) => () => Base[]");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x334 = (n: () => Base[]) => n; x334(function() { 
AssertType(x334, "(() => Base[]) => () => Base[]");
AssertType((n: () => Base[]) => n, "(() => Base[]) => () => Base[]");
AssertType(n, "() => Base[]");
AssertType(n, "() => Base[]");
AssertType(x334(function() { return [d1, d2] }), "() => Base[]");
AssertType(x334, "(() => Base[]) => () => Base[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] });

let x335 = (n: () => Base[]) => n; x335(function named() { 
AssertType(x335, "(() => Base[]) => () => Base[]");
AssertType((n: () => Base[]) => n, "(() => Base[]) => () => Base[]");
AssertType(n, "() => Base[]");
AssertType(n, "() => Base[]");
AssertType(x335(function named() { return [d1, d2] }), "() => Base[]");
AssertType(x335, "(() => Base[]) => () => Base[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] });

let x336 = (n: { (): Base[]; }) => n; x336(() => [d1, d2]);
AssertType(x336, "({    (): Base[];}) => () => Base[]");
AssertType((n: { (): Base[]; }) => n, "({    (): Base[];}) => () => Base[]");
AssertType(n, "() => Base[]");
AssertType(n, "() => Base[]");
AssertType(x336(() => [d1, d2]), "() => Base[]");
AssertType(x336, "(() => Base[]) => () => Base[]");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x337 = (n: { (): Base[]; }) => n; x337(function() { 
AssertType(x337, "({    (): Base[];}) => () => Base[]");
AssertType((n: { (): Base[]; }) => n, "({    (): Base[];}) => () => Base[]");
AssertType(n, "() => Base[]");
AssertType(n, "() => Base[]");
AssertType(x337(function() { return [d1, d2] }), "() => Base[]");
AssertType(x337, "(() => Base[]) => () => Base[]");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] });

let x338 = (n: { (): Base[]; }) => n; x338(function named() { 
AssertType(x338, "({    (): Base[];}) => () => Base[]");
AssertType((n: { (): Base[]; }) => n, "({    (): Base[];}) => () => Base[]");
AssertType(n, "() => Base[]");
AssertType(n, "() => Base[]");
AssertType(x338(function named() { return [d1, d2] }), "() => Base[]");
AssertType(x338, "(() => Base[]) => () => Base[]");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] });

let x339 = (n: Base[]) => n; x339([d1, d2]);
AssertType(x339, "(Base[]) => Base[]");
AssertType((n: Base[]) => n, "(Base[]) => Base[]");
AssertType(n, "Base[]");
AssertType(n, "Base[]");
AssertType(x339([d1, d2]), "Base[]");
AssertType(x339, "(Base[]) => Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x340 = (n: Array<Base>) => n; x340([d1, d2]);
AssertType(x340, "(Array<Base>) => Base[]");
AssertType((n: Array<Base>) => n, "(Array<Base>) => Base[]");
AssertType(n, "Base[]");
AssertType(n, "Base[]");
AssertType(x340([d1, d2]), "Base[]");
AssertType(x340, "(Base[]) => Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x341 = (n: { [n: number]: Base; }) => n; x341([d1, d2]);
AssertType(x341, "({ [number]: Base; }) => { [number]: Base; }");
AssertType((n: { [n: number]: Base; }) => n, "({ [number]: Base; }) => { [number]: Base; }");
AssertType(n, "{ [number]: Base; }");
AssertType(n, "number");
AssertType(n, "{ [number]: Base; }");
AssertType(x341([d1, d2]), "{ [number]: Base; }");
AssertType(x341, "({ [number]: Base; }) => { [number]: Base; }");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x342 = (n: {n: Base[]; } ) => n; x342({ n: [d1, d2] });
AssertType(x342, "({ n: Base[]; }) => { n: Base[]; }");
AssertType((n: {n: Base[]; } ) => n, "({ n: Base[]; }) => { n: Base[]; }");
AssertType(n, "{ n: Base[]; }");
AssertType(n, "Base[]");
AssertType(n, "{ n: Base[]; }");
AssertType(x342({ n: [d1, d2] }), "{ n: Base[]; }");
AssertType(x342, "({ n: Base[]; }) => { n: Base[]; }");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x343 = (n: (s: Base[]) => any) => n; x343(n => { let n: Base[]; 
AssertType(x343, "((Base[]) => any) => (Base[]) => any");
AssertType((n: (s: Base[]) => any) => n, "((Base[]) => any) => (Base[]) => any");
AssertType(n, "(Base[]) => any");
AssertType(s, "Base[]");
AssertType(n, "(Base[]) => any");
AssertType(x343(n => { let n: Base[]; return null; }), "(Base[]) => any");
AssertType(x343, "((Base[]) => any) => (Base[]) => any");
AssertType(n => { let n: Base[]; return null; }, "(Base[]) => any");
AssertType(n, "Base[]");
AssertType(n, "Base[]");
AssertType(null, "null");
return null; });

let x344 = (n: Genric<Base>) => n; x344({ func: n => { 
AssertType(x344, "(Genric<Base>) => Genric<Base>");
AssertType((n: Genric<Base>) => n, "(Genric<Base>) => Genric<Base>");
AssertType(n, "Genric<Base>");
AssertType(n, "Genric<Base>");
AssertType(x344({ func: n => { return [d1, d2]; } }), "Genric<Base>");
AssertType(x344, "(Genric<Base>) => Genric<Base>");
AssertType({ func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
AssertType(func, "(Base[]) => (union)[]");
AssertType(n => { return [d1, d2]; }, "(Base[]) => (union)[]");
AssertType(n, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2]; } });

let x345 = function(n: () => Base[]) { }; x345(() => [d1, d2]);
AssertType(x345, "(() => Base[]) => void");
AssertType(function(n: () => Base[]) { }, "(() => Base[]) => void");
AssertType(n, "() => Base[]");
AssertType(x345(() => [d1, d2]), "void");
AssertType(x345, "(() => Base[]) => void");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x346 = function(n: () => Base[]) { }; x346(function() { 
AssertType(x346, "(() => Base[]) => void");
AssertType(function(n: () => Base[]) { }, "(() => Base[]) => void");
AssertType(n, "() => Base[]");
AssertType(x346(function() { return [d1, d2] }), "void");
AssertType(x346, "(() => Base[]) => void");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] });

let x347 = function(n: () => Base[]) { }; x347(function named() { 
AssertType(x347, "(() => Base[]) => void");
AssertType(function(n: () => Base[]) { }, "(() => Base[]) => void");
AssertType(n, "() => Base[]");
AssertType(x347(function named() { return [d1, d2] }), "void");
AssertType(x347, "(() => Base[]) => void");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] });

let x348 = function(n: { (): Base[]; }) { }; x348(() => [d1, d2]);
AssertType(x348, "({    (): Base[];}) => void");
AssertType(function(n: { (): Base[]; }) { }, "({    (): Base[];}) => void");
AssertType(n, "() => Base[]");
AssertType(x348(() => [d1, d2]), "void");
AssertType(x348, "(() => Base[]) => void");
AssertType(() => [d1, d2], "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x349 = function(n: { (): Base[]; }) { }; x349(function() { 
AssertType(x349, "({    (): Base[];}) => void");
AssertType(function(n: { (): Base[]; }) { }, "({    (): Base[];}) => void");
AssertType(n, "() => Base[]");
AssertType(x349(function() { return [d1, d2] }), "void");
AssertType(x349, "(() => Base[]) => void");
AssertType(function() { return [d1, d2] }, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] });

let x350 = function(n: { (): Base[]; }) { }; x350(function named() { 
AssertType(x350, "({    (): Base[];}) => void");
AssertType(function(n: { (): Base[]; }) { }, "({    (): Base[];}) => void");
AssertType(n, "() => Base[]");
AssertType(x350(function named() { return [d1, d2] }), "void");
AssertType(x350, "(() => Base[]) => void");
AssertType(function named() { return [d1, d2] }, "() => (union)[]");
AssertType(named, "() => (union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2] });

let x351 = function(n: Base[]) { }; x351([d1, d2]);
AssertType(x351, "(Base[]) => void");
AssertType(function(n: Base[]) { }, "(Base[]) => void");
AssertType(n, "Base[]");
AssertType(x351([d1, d2]), "void");
AssertType(x351, "(Base[]) => void");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x352 = function(n: Array<Base>) { }; x352([d1, d2]);
AssertType(x352, "(Array<Base>) => void");
AssertType(function(n: Array<Base>) { }, "(Array<Base>) => void");
AssertType(n, "Base[]");
AssertType(x352([d1, d2]), "void");
AssertType(x352, "(Base[]) => void");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x353 = function(n: { [n: number]: Base; }) { }; x353([d1, d2]);
AssertType(x353, "({ [number]: Base; }) => void");
AssertType(function(n: { [n: number]: Base; }) { }, "({ [number]: Base; }) => void");
AssertType(n, "{ [number]: Base; }");
AssertType(n, "number");
AssertType(x353([d1, d2]), "void");
AssertType(x353, "({ [number]: Base; }) => void");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x354 = function(n: {n: Base[]; } ) { }; x354({ n: [d1, d2] });
AssertType(x354, "({ n: Base[]; }) => void");
AssertType(function(n: {n: Base[]; } ) { }, "({ n: Base[]; }) => void");
AssertType(n, "{ n: Base[]; }");
AssertType(n, "Base[]");
AssertType(x354({ n: [d1, d2] }), "void");
AssertType(x354, "({ n: Base[]; }) => void");
AssertType({ n: [d1, d2] }, "{ n: (union)[]; }");
AssertType(n, "(union)[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");

let x355 = function(n: (s: Base[]) => any) { }; x355(n => { let n: Base[]; 
AssertType(x355, "((Base[]) => any) => void");
AssertType(function(n: (s: Base[]) => any) { }, "((Base[]) => any) => void");
AssertType(n, "(Base[]) => any");
AssertType(s, "Base[]");
AssertType(x355(n => { let n: Base[]; return null; }), "void");
AssertType(x355, "((Base[]) => any) => void");
AssertType(n => { let n: Base[]; return null; }, "(Base[]) => any");
AssertType(n, "Base[]");
AssertType(n, "Base[]");
AssertType(null, "null");
return null; });

let x356 = function(n: Genric<Base>) { }; x356({ func: n => { 
AssertType(x356, "(Genric<Base>) => void");
AssertType(function(n: Genric<Base>) { }, "(Genric<Base>) => void");
AssertType(n, "Genric<Base>");
AssertType(x356({ func: n => { return [d1, d2]; } }), "void");
AssertType(x356, "(Genric<Base>) => void");
AssertType({ func: n => { return [d1, d2]; } }, "{ func: (Base[]) => (union)[]; }");
AssertType(func, "(Base[]) => (union)[]");
AssertType(n => { return [d1, d2]; }, "(Base[]) => (union)[]");
AssertType(n, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived1");
AssertType(d2, "Derived2");
return [d1, d2]; } });


