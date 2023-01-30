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

// === tests/cases/conformance/types/uniqueSymbol/uniqueSymbolsDeclarations.ts ===
declare function AssertType(value:any, type:string):void;
// declarations with call initializer
const constCall = Symbol();
AssertType(constCall, "unique symbol");
AssertType(Symbol(), "unique symbol");
AssertType(Symbol, "SymbolConstructor");

let letCall = Symbol();
AssertType(letCall, "symbol");
AssertType(Symbol(), "symbol");
AssertType(Symbol, "SymbolConstructor");

let letCall = Symbol();
AssertType(letCall, "symbol");
AssertType(Symbol(), "symbol");
AssertType(Symbol, "SymbolConstructor");

// ambient declaration with type
declare const constType: unique symbol;
AssertType(constType, "unique symbol");

// declaration with type and call initializer
const constTypeAndCall: unique symbol = Symbol();
AssertType(constTypeAndCall, "unique symbol");
AssertType(Symbol(), "unique symbol");
AssertType(Symbol, "SymbolConstructor");

// declaration from initializer
const constInitToConstCall = constCall;
AssertType(constInitToConstCall, "symbol");
AssertType(constCall, "unique symbol");

const constInitToLetCall = letCall;
AssertType(constInitToLetCall, "symbol");
AssertType(letCall, "symbol");

const constInitToVarCall = letCall;
AssertType(constInitToVarCall, "symbol");
AssertType(letCall, "symbol");

const constInitToConstDeclAmbient = constType;
AssertType(constInitToConstDeclAmbient, "symbol");
AssertType(constType, "unique symbol");

let letInitToConstCall = constCall;
AssertType(letInitToConstCall, "symbol");
AssertType(constCall, "unique symbol");

let letInitToLetCall = letCall;
AssertType(letInitToLetCall, "symbol");
AssertType(letCall, "symbol");

let letInitToVarCall = letCall;
AssertType(letInitToVarCall, "symbol");
AssertType(letCall, "symbol");

let letInitToConstDeclAmbient = constType;
AssertType(letInitToConstDeclAmbient, "symbol");
AssertType(constType, "unique symbol");

let letInitToConstCall = constCall;
AssertType(letInitToConstCall, "symbol");
AssertType(constCall, "unique symbol");

let letInitToLetCall = letCall;
AssertType(letInitToLetCall, "symbol");
AssertType(letCall, "symbol");

let letInitToVarCall = letCall;
AssertType(letInitToVarCall, "symbol");
AssertType(letCall, "symbol");

let letInitToConstDeclAmbient = constType;
AssertType(letInitToConstDeclAmbient, "symbol");
AssertType(constType, "unique symbol");

// declaration from initializer with type query
const constInitToConstCallWithTypeQuery: typeof constCall = constCall;
AssertType(constInitToConstCallWithTypeQuery, "unique symbol");
AssertType(constCall, "unique symbol");
AssertType(constCall, "unique symbol");

const constInitToConstDeclAmbientWithTypeQuery: typeof constType = constType;
AssertType(constInitToConstDeclAmbientWithTypeQuery, "unique symbol");
AssertType(constType, "unique symbol");
AssertType(constType, "unique symbol");

// function return inference
function funcReturnConstCall() { 
AssertType(constCall, "unique symbol");
return constCall; 
}

function funcReturnLetCall() { 
AssertType(letCall, "symbol");
return letCall; 
}

function funcReturnVarCall() { 
AssertType(letCall, "symbol");
return letCall; 
}

// function return value with type query
function funcReturnConstCallWithTypeQuery(): typeof constCall { 
AssertType(constCall, "unique symbol");
return constCall; 
}

// generator function yield inference
function* genFuncYieldConstCall() { yield constCall; 
AssertType(yield constCall, "any");

AssertType(constCall, "unique symbol");
}

function* genFuncYieldLetCall() { yield letCall; 
AssertType(yield letCall, "any");

AssertType(letCall, "symbol");
}

function* genFuncYieldVarCall() { yield letCall; 
AssertType(yield letCall, "any");

AssertType(letCall, "symbol");
}

// generator function yield with return type query
function* genFuncYieldConstCallWithTypeQuery(): IterableIterator<typeof constCall> { yield constCall; 
AssertType(yield constCall, "undefined");

AssertType(constCall, "unique symbol");
}

// async function return inference
async function asyncFuncReturnConstCall() { 
AssertType(constCall, "unique symbol");
return constCall; 
}

async function asyncFuncReturnLetCall() { 
AssertType(letCall, "symbol");
return letCall; 
}

async function asyncFuncReturnVarCall() { 
AssertType(letCall, "symbol");
return letCall; 
}

// async generator function yield inference
async function* asyncGenFuncYieldConstCall() { yield constCall; 
AssertType(yield constCall, "any");

AssertType(constCall, "unique symbol");
}

async function* asyncGenFuncYieldLetCall() { yield letCall; 
AssertType(yield letCall, "any");

AssertType(letCall, "symbol");
}

async function* asyncGenFuncYieldVarCall() { yield letCall; 
AssertType(yield letCall, "any");

AssertType(letCall, "symbol");
}

// classes
class C {
    static readonly readonlyStaticCall = Symbol();
    static readonly readonlyStaticType: unique symbol;
    static readonly readonlyStaticTypeAndCall: unique symbol = Symbol();
    static readwriteStaticCall = Symbol();

    readonly readonlyCall = Symbol();
    readwriteCall = Symbol();
}
declare const c: C;
AssertType(c, "C");

const constInitToCReadonlyStaticCall = C.readonlyStaticCall;
AssertType(constInitToCReadonlyStaticCall, "symbol");
AssertType(C.readonlyStaticCall, "unique symbol");

const constInitToCReadonlyStaticType = C.readonlyStaticType;
AssertType(constInitToCReadonlyStaticType, "symbol");
AssertType(C.readonlyStaticType, "unique symbol");

const constInitToCReadonlyStaticTypeAndCall = C.readonlyStaticTypeAndCall;
AssertType(constInitToCReadonlyStaticTypeAndCall, "symbol");
AssertType(C.readonlyStaticTypeAndCall, "unique symbol");

const constInitToCReadwriteStaticCall = C.readwriteStaticCall;
AssertType(constInitToCReadwriteStaticCall, "symbol");
AssertType(C.readwriteStaticCall, "symbol");

const constInitToCReadonlyStaticCallWithTypeQuery: typeof C.readonlyStaticCall = C.readonlyStaticCall;
AssertType(constInitToCReadonlyStaticCallWithTypeQuery, "unique symbol");
AssertType(C.readonlyStaticCall, "unique symbol");
AssertType(C, "typeof C");
AssertType(readonlyStaticCall, "unique symbol");
AssertType(C.readonlyStaticCall, "unique symbol");

const constInitToCReadonlyStaticTypeWithTypeQuery: typeof C.readonlyStaticType = C.readonlyStaticType;
AssertType(constInitToCReadonlyStaticTypeWithTypeQuery, "unique symbol");
AssertType(C.readonlyStaticType, "unique symbol");
AssertType(C, "typeof C");
AssertType(readonlyStaticType, "unique symbol");
AssertType(C.readonlyStaticType, "unique symbol");

const constInitToCReadonlyStaticTypeAndCallWithTypeQuery: typeof C.readonlyStaticTypeAndCall = C.readonlyStaticTypeAndCall;
AssertType(constInitToCReadonlyStaticTypeAndCallWithTypeQuery, "unique symbol");
AssertType(C.readonlyStaticTypeAndCall, "unique symbol");
AssertType(C, "typeof C");
AssertType(readonlyStaticTypeAndCall, "unique symbol");
AssertType(C.readonlyStaticTypeAndCall, "unique symbol");

const constInitToCReadwriteStaticCallWithTypeQuery: typeof C.readwriteStaticCall = C.readwriteStaticCall;
AssertType(constInitToCReadwriteStaticCallWithTypeQuery, "symbol");
AssertType(C.readwriteStaticCall, "symbol");
AssertType(C, "typeof C");
AssertType(readwriteStaticCall, "symbol");
AssertType(C.readwriteStaticCall, "symbol");

const constInitToCReadonlyCall = c.readonlyCall;
AssertType(constInitToCReadonlyCall, "symbol");
AssertType(c.readonlyCall, "symbol");

const constInitToCReadwriteCall = c.readwriteCall;
AssertType(constInitToCReadwriteCall, "symbol");
AssertType(c.readwriteCall, "symbol");

const constInitToCReadonlyCallWithTypeQuery: typeof c.readonlyCall = c.readonlyCall;
AssertType(constInitToCReadonlyCallWithTypeQuery, "symbol");
AssertType(c.readonlyCall, "symbol");
AssertType(c, "C");
AssertType(readonlyCall, "symbol");
AssertType(c.readonlyCall, "symbol");

const constInitToCReadwriteCallWithTypeQuery: typeof c.readwriteCall = c.readwriteCall;
AssertType(constInitToCReadwriteCallWithTypeQuery, "symbol");
AssertType(c.readwriteCall, "symbol");
AssertType(c, "C");
AssertType(readwriteCall, "symbol");
AssertType(c.readwriteCall, "symbol");

const constInitToCReadonlyCallWithIndexedAccess: C["readonlyCall"] = c.readonlyCall;
AssertType(constInitToCReadonlyCallWithIndexedAccess, "symbol");
AssertType(c.readonlyCall, "symbol");

const constInitToCReadwriteCallWithIndexedAccess: C["readwriteCall"] = c.readwriteCall;
AssertType(constInitToCReadwriteCallWithIndexedAccess, "symbol");
AssertType(c.readwriteCall, "symbol");

// interfaces
interface I {
    readonly readonlyType: unique symbol;
}
declare const i: I;
AssertType(i, "I");

const constInitToIReadonlyType = i.readonlyType;
AssertType(constInitToIReadonlyType, "symbol");
AssertType(i.readonlyType, "unique symbol");

const constInitToIReadonlyTypeWithTypeQuery: typeof i.readonlyType = i.readonlyType;
AssertType(constInitToIReadonlyTypeWithTypeQuery, "unique symbol");
AssertType(i.readonlyType, "unique symbol");
AssertType(i, "I");
AssertType(readonlyType, "unique symbol");
AssertType(i.readonlyType, "unique symbol");

const constInitToIReadonlyTypeWithIndexedAccess: I["readonlyType"] = i.readonlyType;
AssertType(constInitToIReadonlyTypeWithIndexedAccess, "unique symbol");
AssertType(i.readonlyType, "unique symbol");

// type literals
type L = {
    readonly readonlyType: unique symbol;
    nested: {
        readonly readonlyNestedType: unique symbol;
    }
};
declare const l: L;
AssertType(l, "L");

const constInitToLReadonlyType = l.readonlyType;
AssertType(constInitToLReadonlyType, "symbol");
AssertType(l.readonlyType, "unique symbol");

const constInitToLReadonlyNestedType = l.nested.readonlyNestedType;
AssertType(constInitToLReadonlyNestedType, "symbol");
AssertType(l.nested.readonlyNestedType, "unique symbol");

const constInitToLReadonlyTypeWithTypeQuery: typeof l.readonlyType = l.readonlyType;
AssertType(constInitToLReadonlyTypeWithTypeQuery, "unique symbol");
AssertType(l.readonlyType, "unique symbol");
AssertType(l, "L");
AssertType(readonlyType, "unique symbol");
AssertType(l.readonlyType, "unique symbol");

const constInitToLReadonlyNestedTypeWithTypeQuery: typeof l.nested.readonlyNestedType = l.nested.readonlyNestedType;
AssertType(constInitToLReadonlyNestedTypeWithTypeQuery, "unique symbol");
AssertType(l.nested.readonlyNestedType, "unique symbol");
AssertType(l.nested, "{ readonly readonlyNestedType: unique symbol; }");
AssertType(l, "L");
AssertType(nested, "{ readonly readonlyNestedType: unique symbol; }");
AssertType(readonlyNestedType, "unique symbol");
AssertType(l.nested.readonlyNestedType, "unique symbol");

const constInitToLReadonlyTypeWithIndexedAccess: L["readonlyType"] = l.readonlyType;
AssertType(constInitToLReadonlyTypeWithIndexedAccess, "unique symbol");
AssertType(l.readonlyType, "unique symbol");

const constInitToLReadonlyNestedTypeWithIndexedAccess: L["nested"]["readonlyNestedType"] = l.nested.readonlyNestedType;
AssertType(constInitToLReadonlyNestedTypeWithIndexedAccess, "unique symbol");
AssertType(l.nested.readonlyNestedType, "unique symbol");

// type argument inference
const promiseForConstCall = Promise.resolve(constCall);
AssertType(promiseForConstCall, "Promise<unique symbol>");
AssertType(Promise.resolve(constCall), "Promise<unique symbol>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(constCall, "unique symbol");

const arrayOfConstCall = [constCall];
AssertType(arrayOfConstCall, "symbol[]");
AssertType([constCall], "symbol[]");
AssertType(constCall, "unique symbol");

// unique symbol widening in expressions
declare const s: unique symbol;
AssertType(s, "unique symbol");

declare namespace N { const s: unique symbol; }
declare const o: { [s]: "a", [N.s]: "b" };
AssertType(o, "{ [s]: "a"; [N.s]: "b"; }");
AssertType([s], "string");
AssertType(s, "unique symbol");
AssertType([N.s], "string");
AssertType(N.s, "unique symbol");

declare function f<T>(x: T): T;
declare function g(x: typeof s): void;
declare function g(x: typeof N.s): void;

// widening positions

// argument inference
f(s);
AssertType(f(s), "unique symbol");
AssertType(f, "<T>(T) => T");
AssertType(s, "unique symbol");

f(N.s);
AssertType(f(N.s), "unique symbol");
AssertType(f, "<T>(T) => T");
AssertType(N.s, "unique symbol");

f(N["s"]);
AssertType(f(N["s"]), "unique symbol");
AssertType(f, "<T>(T) => T");
AssertType(N["s"], "unique symbol");
AssertType(N, "typeof N");
AssertType("s", "string");

// array literal elements
[s];
AssertType([s], "symbol[]");
AssertType(s, "unique symbol");

[N.s];
AssertType([N.s], "symbol[]");
AssertType(N.s, "unique symbol");

[N["s"]];
AssertType([N["s"]], "symbol[]");
AssertType(N["s"], "unique symbol");
AssertType(N, "typeof N");
AssertType("s", "string");

// property assignments/methods
const o2 = {
AssertType(o2, "{ a: symbol; b: symbol; c: symbol; method1(): symbol; method2(): Promise<symbol>; method3(): AsyncGenerator<symbol, void, unknown>; method4(): Generator<symbol, void, unknown>; method5(?symbol): symbol; }");
AssertType({    a: s,    b: N.s,    c: N["s"],    method1() { return s; },    async method2() { return s; },    async * method3() { yield s; },    * method4() { yield s; },    method5(p = s) { return p; }}, "{ a: symbol; b: symbol; c: symbol; method1(): symbol; method2(): Promise<symbol>; method3(): AsyncGenerator<symbol, void, unknown>; method4(): Generator<symbol, void, unknown>; method5(?symbol): symbol; }");

    a: s,
AssertType(a, "symbol");
AssertType(s, "unique symbol");

    b: N.s,
AssertType(b, "symbol");
AssertType(N.s, "unique symbol");

    c: N["s"],
AssertType(c, "symbol");
AssertType(N["s"], "unique symbol");
AssertType(N, "typeof N");
AssertType("s", "string");

    method1() { 
AssertType(method1, "() => symbol");
AssertType(s, "unique symbol");
return s; },

    async method2() { 
AssertType(method2, "() => Promise<symbol>");
AssertType(s, "unique symbol");
return s; },

    async * method3() { yield s; },
AssertType(method3, "() => AsyncGenerator<symbol, void, unknown>");
AssertType(yield s, "any");
AssertType(s, "unique symbol");

    * method4() { yield s; },
AssertType(method4, "() => Generator<symbol, void, unknown>");
AssertType(yield s, "any");
AssertType(s, "unique symbol");

    method5(p = s) { 
AssertType(method5, "(?symbol) => symbol");
return p; 

AssertType(p, "symbol");

AssertType(s, "unique symbol");

AssertType(p, "symbol");
}

};

// property initializers
class C0 {
    static readonly a = s;
    static readonly b = N.s;
    static readonly c = N["s"];

    static d = s;
    static e = N.s;
    static f = N["s"];

    readonly a = s;
    readonly b = N.s;
    readonly c = N["s"];

    d = s;
    e = N.s;
    f = N["s"];

    method1() { 
AssertType(s, "unique symbol");
return s; 
}

    async method2() { 
AssertType(s, "unique symbol");
return s; 
}

    async * method3() { yield s; 
AssertType(yield s, "any");

AssertType(s, "unique symbol");
}

    * method4() { yield s; 
AssertType(yield s, "any");

AssertType(s, "unique symbol");
}

    method5(p = s) { 
AssertType(p, "symbol");
return p; 
}
}

// non-widening positions

// element access
o[s];
AssertType(o[s], "string");
AssertType(o, "{ [s]: "a"; [N.s]: "b"; }");
AssertType(s, "unique symbol");

o[N.s];
AssertType(o[N.s], "string");
AssertType(o, "{ [s]: "a"; [N.s]: "b"; }");
AssertType(N.s, "unique symbol");

o[N["s"]];
AssertType(o[N["s"]], "string");
AssertType(o, "{ [s]: "a"; [N.s]: "b"; }");
AssertType(N["s"], "unique symbol");
AssertType(N, "typeof N");
AssertType("s", "string");

// arguments (no-inference)
f<typeof s>(s);
AssertType(f<typeof s>(s), "unique symbol");
AssertType(f, "<T>(T) => T");
AssertType(s, "unique symbol");
AssertType(s, "unique symbol");

f<typeof N.s>(N.s);
AssertType(f<typeof N.s>(N.s), "unique symbol");
AssertType(f, "<T>(T) => T");
AssertType(N.s, "unique symbol");
AssertType(N, "typeof N");
AssertType(s, "unique symbol");
AssertType(N.s, "unique symbol");

f<typeof N.s>(N["s"]);
AssertType(f<typeof N.s>(N["s"]), "unique symbol");
AssertType(f, "<T>(T) => T");
AssertType(N.s, "unique symbol");
AssertType(N, "typeof N");
AssertType(s, "unique symbol");
AssertType(N["s"], "unique symbol");
AssertType(N, "typeof N");
AssertType("s", "string");

g(s);
AssertType(g(s), "void");
AssertType(g, "{ (unique symbol): void; (unique symbol): void; }");
AssertType(s, "unique symbol");

g(N.s);
AssertType(g(N.s), "void");
AssertType(g, "{ (unique symbol): void; (unique symbol): void; }");
AssertType(N.s, "unique symbol");

g(N["s"]);
AssertType(g(N["s"]), "void");
AssertType(g, "{ (unique symbol): void; (unique symbol): void; }");
AssertType(N["s"], "unique symbol");
AssertType(N, "typeof N");
AssertType("s", "string");

// falsy expressions
s || "";
AssertType(s || "", "union");
AssertType(s, "unique symbol");
AssertType("", "string");

N.s || "";
AssertType(N.s || "", "union");
AssertType(N.s, "unique symbol");
AssertType("", "string");

N["s"] || "";
AssertType(N["s"] || "", "union");
AssertType(N["s"], "unique symbol");
AssertType(N, "typeof N");
AssertType("s", "string");
AssertType("", "string");

// conditionals
Math.random() * 2 ? s : "a";
AssertType(Math.random() * 2 ? s : "a", "union");
AssertType(Math.random() * 2, "number");
AssertType(Math.random(), "number");
AssertType(Math.random, "() => number");
AssertType(2, "int");
AssertType(s, "unique symbol");
AssertType("a", "string");

Math.random() * 2 ? N.s : "a";
AssertType(Math.random() * 2 ? N.s : "a", "union");
AssertType(Math.random() * 2, "number");
AssertType(Math.random(), "number");
AssertType(Math.random, "() => number");
AssertType(2, "int");
AssertType(N.s, "unique symbol");
AssertType("a", "string");

Math.random() * 2 ? N["s"] : "a";
AssertType(Math.random() * 2 ? N["s"] : "a", "union");
AssertType(Math.random() * 2, "number");
AssertType(Math.random(), "number");
AssertType(Math.random, "() => number");
AssertType(2, "int");
AssertType(N["s"], "unique symbol");
AssertType(N, "typeof N");
AssertType("s", "string");
AssertType("a", "string");

// computed property names
({
AssertType(({    [s]: "a",    [N.s]: "b",}), "{ [s]: string; [N.s]: string; }");
AssertType({    [s]: "a",    [N.s]: "b",}, "{ [s]: string; [N.s]: string; }");

    [s]: "a",
AssertType([s], "string");
AssertType(s, "unique symbol");
AssertType("a", "string");

    [N.s]: "b",
AssertType([N.s], "string");
AssertType(N.s, "unique symbol");
AssertType("b", "string");

});

class C1 {
    static [s]: "a";
    static [N.s]: "b";

    [s]: "a";
    [N.s]: "b";
}

// contextual types

interface Context {
    method1(): typeof s;
    method2(): Promise<typeof s>;
    method3(): AsyncIterableIterator<typeof s>;
    method4(): IterableIterator<typeof s>;
    method5(p?: typeof s): typeof s;
}

const o4: Context = {
AssertType(o4, "Context");
AssertType({    method1() {        return s; // return type should not widen due to contextual type    },    async method2() {        return s; // return type should not widen due to contextual type    },    async * method3() {        yield s; // yield type should not widen due to contextual type    },    * method4() {        yield s; // yield type should not widen due to contextual type    },    method5(p = s) { // parameter should not widen due to contextual type        return p;    }}, "{ method1(): unique symbol; method2(): Promise<unique symbol>; method3(): AsyncGenerator<unique symbol, void, undefined>; method4(): Generator<unique symbol, void, undefined>; method5(?unique symbol): unique symbol; }");

    method1() {
AssertType(method1, "() => unique symbol");

AssertType(s, "unique symbol");
        return s; // return type should not widen due to contextual type

    },
    async method2() {
AssertType(method2, "() => Promise<unique symbol>");

AssertType(s, "unique symbol");
        return s; // return type should not widen due to contextual type

    },
    async * method3() {
AssertType(method3, "() => AsyncGenerator<unique symbol, void, undefined>");

        yield s; // yield type should not widen due to contextual type
AssertType(yield s, "undefined");
AssertType(s, "unique symbol");

    },
    * method4() {
AssertType(method4, "() => Generator<unique symbol, void, undefined>");

        yield s; // yield type should not widen due to contextual type
AssertType(yield s, "undefined");
AssertType(s, "unique symbol");

    },
    method5(p = s) { // parameter should not widen due to contextual type
AssertType(method5, "(?unique symbol) => unique symbol");
AssertType(p, "unique symbol");
AssertType(s, "unique symbol");

AssertType(p, "unique symbol");
        return p;
    }
};

