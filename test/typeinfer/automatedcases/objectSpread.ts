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

// === tests/cases/conformance/types/spread/objectSpread.ts ===
declare function AssertType(value:any, type:string):void;
let o = { a: 1, b: 'no' 
AssertType(o, "{ a: number; b: string; }");

AssertType({ a: 1, b: 'no' }, "{ a: number; b: string; }");

AssertType(a, "number");

AssertType(1, "int");

AssertType(b, "string");

AssertType('no', "string");
}

let o2 = { b: 'yes', c: true 
AssertType(o2, "{ b: string; c: boolean; }");

AssertType({ b: 'yes', c: true }, "{ b: string; c: boolean; }");

AssertType(b, "string");

AssertType('yes', "string");

AssertType(c, "boolean");

AssertType(true, "boolean");
}

let swap = { a: 'yes', b: -1 };
AssertType(swap, "{ a: string; b: number; }");
AssertType({ a: 'yes', b: -1 }, "{ a: string; b: number; }");
AssertType(a, "string");
AssertType('yes', "string");
AssertType(b, "number");
AssertType(-1, "int");
AssertType(1, "int");

let addAfter: { a: number, b: string, c: boolean } =
AssertType(addAfter, "{ a: number; b: string; c: boolean; }");
AssertType(a, "number");
AssertType(b, "string");
AssertType(c, "boolean");

    { ...o, c: false 
AssertType({ ...o, c: false }, "{ c: false; a: number; b: string; }");

AssertType(o, "{ a: number; b: string; }");

AssertType(c, "boolean");

AssertType(false, "boolean");
}

let addBefore: { a: number, b: string, c: boolean } =
AssertType(addBefore, "{ a: number; b: string; c: boolean; }");
AssertType(a, "number");
AssertType(b, "string");
AssertType(c, "boolean");

    { c: false, ...o 
AssertType({ c: false, ...o }, "{ a: number; b: string; c: false; }");

AssertType(c, "boolean");

AssertType(false, "boolean");

AssertType(o, "{ a: number; b: string; }");
}

let override: { a: number, b: string } =
AssertType(override, "{ a: number; b: string; }");
AssertType(a, "number");
AssertType(b, "string");

    { ...o, b: 'override' 
AssertType({ ...o, b: 'override' }, "{ b: string; a: number; }");

AssertType(o, "{ a: number; b: string; }");

AssertType(b, "string");

AssertType('override', "string");
}

let nested: { a: number, b: boolean, c: string } =
AssertType(nested, "{ a: number; b: boolean; c: string; }");
AssertType(a, "number");
AssertType(b, "boolean");
AssertType(c, "string");

    { ...{ a: 3, ...{ b: false, c: 'overriden' } }, c: 'whatever' 
AssertType({ ...{ a: 3, ...{ b: false, c: 'overriden' } }, c: 'whatever' }, "{ c: string; b: false; a: number; }");

AssertType({ a: 3, ...{ b: false, c: 'overriden' } }, "{ b: false; c: string; a: number; }");

AssertType(a, "number");

AssertType(3, "int");

AssertType({ b: false, c: 'overriden' }, "{ b: false; c: string; }");

AssertType(b, "boolean");

AssertType(false, "boolean");

AssertType(c, "string");

AssertType('overriden', "string");

AssertType(c, "string");

AssertType('whatever', "string");
}

let combined: { a: number, b: string, c: boolean } =
AssertType(combined, "{ a: number; b: string; c: boolean; }");
AssertType(a, "number");
AssertType(b, "string");
AssertType(c, "boolean");

    { ...o, ...o2 
AssertType({ ...o, ...o2 }, "{ b: string; c: boolean; a: number; }");

AssertType(o, "{ a: number; b: string; }");

AssertType(o2, "{ b: string; c: boolean; }");
}

let combinedAfter: { a: number, b: string, c: boolean } =
AssertType(combinedAfter, "{ a: number; b: string; c: boolean; }");
AssertType(a, "number");
AssertType(b, "string");
AssertType(c, "boolean");

    { ...o, ...o2, b: 'ok' 
AssertType({ ...o, ...o2, b: 'ok' }, "{ b: string; c: boolean; a: number; }");

AssertType(o, "{ a: number; b: string; }");

AssertType(o2, "{ b: string; c: boolean; }");

AssertType(b, "string");

AssertType('ok', "string");
}

let combinedNestedChangeType: { a: number, b: boolean, c: number } =
AssertType(combinedNestedChangeType, "{ a: number; b: boolean; c: number; }");
AssertType(a, "number");
AssertType(b, "boolean");
AssertType(c, "number");

    { ...{ a: 1, ...{ b: false, c: 'overriden' } }, c: -1 
AssertType({ ...{ a: 1, ...{ b: false, c: 'overriden' } }, c: -1 }, "{ c: number; b: false; a: number; }");

AssertType({ a: 1, ...{ b: false, c: 'overriden' } }, "{ b: false; c: string; a: number; }");

AssertType(a, "number");

AssertType(1, "int");

AssertType({ b: false, c: 'overriden' }, "{ b: false; c: string; }");

AssertType(b, "boolean");

AssertType(false, "boolean");

AssertType(c, "string");

AssertType('overriden', "string");

AssertType(c, "number");

AssertType(-1, "int");

AssertType(1, "int");
}

let propertyNested: { a: { a: number, b: string } } =
AssertType(propertyNested, "{ a: {    a: number;    b: string;}; }");
AssertType(a, "{ a: number; b: string; }");
AssertType(a, "number");
AssertType(b, "string");

    { a: { ... o } 
AssertType({ a: { ... o } }, "{ a: { a: number; b: string; }; }");

AssertType(a, "{ a: number; b: string; }");

AssertType({ ... o }, "{ a: number; b: string; }");

AssertType(o, "{ a: number; b: string; }");
}

// accessors don't copy the descriptor
// (which means that readonly getters become read/write properties)
let op = { get a () { 
AssertType(op, "{ readonly a: number; }");
AssertType({ get a () { return 6 } }, "{ readonly a: number; }");
AssertType(a, "number");
AssertType(6, "int");
return 6 } };

let getter: { a: number, c: number } =
AssertType(getter, "{ a: number; c: number; }");
AssertType(a, "number");
AssertType(c, "number");

    { ...op, c: 7 
AssertType({ ...op, c: 7 }, "{ c: number; a: number; }");

AssertType(op, "{ readonly a: number; }");

AssertType(c, "number");

AssertType(7, "int");
}

getter.a = 12;
AssertType(getter.a = 12, "int");
AssertType(getter.a, "number");
AssertType(12, "int");

// functions result in { }
let spreadFunc = { ...(function () { }) };
AssertType(spreadFunc, "{}");
AssertType({ ...(function () { }) }, "{}");
AssertType((function () { }), "() => void");
AssertType(function () { }, "() => void");

type Header = { head: string, body: string, authToken: string }
function from16326(this: { header: Header }, header: Header, authToken: string): Header {
AssertType({        ...this.header,        ...header,        ...authToken && { authToken }    }, "{ authToken: string; head: string; body: string; }");
    return {

        ...this.header,
AssertType(this.header, "Header");
AssertType(this, "{ header: Header; }");

        ...header,
AssertType(header, "Header");

        ...authToken && { authToken 
AssertType(authToken && { authToken }, "union");

AssertType(authToken, "string");

AssertType({ authToken }, "{ authToken: string; }");

AssertType(authToken, "string");
}
    }
}
// boolean && T results in Partial<T>
function conditionalSpreadBoolean(b: boolean) : { x: number, y: number } {
    let o = { x: 12, y: 13 
AssertType(o, "{ x: number; y: number; }");

AssertType({ x: 12, y: 13 }, "{ x: number; y: number; }");

AssertType(x, "number");

AssertType(12, "int");

AssertType(y, "number");

AssertType(13, "int");
}

    o = {
AssertType(o = {        ...o,        ...b && { x: 14 }    }, "{ x: number; y: number; }");
AssertType(o, "{ x: number; y: number; }");
AssertType({        ...o,        ...b && { x: 14 }    }, "{ x: number; y: number; }");

        ...o,
AssertType(o, "{ x: number; y: number; }");

        ...b && { x: 14 
AssertType(b && { x: 14 }, "union");

AssertType(b, "boolean");

AssertType({ x: 14 }, "{ x: number; }");

AssertType(x, "number");

AssertType(14, "int");
}
    }
    let o2 = { ...b && { x: 21 }
AssertType(o2, "{ x?: union; }");

AssertType({ ...b && { x: 21 }}, "{ x?: union; }");

AssertType(b && { x: 21 }, "union");

AssertType(b, "boolean");

AssertType({ x: 21 }, "{ x: number; }");

AssertType(x, "number");

AssertType(21, "int");
}

AssertType(o, "{ x: number; y: number; }");
    return o;
}
function conditionalSpreadNumber(nt: number): { x: number, y: number } {
    let o = { x: 15, y: 16 
AssertType(o, "{ x: number; y: number; }");

AssertType({ x: 15, y: 16 }, "{ x: number; y: number; }");

AssertType(x, "number");

AssertType(15, "int");

AssertType(y, "number");

AssertType(16, "int");
}

    o = {
AssertType(o = {        ...o,        ...nt && { x: nt }    }, "{ x: number; y: number; }");
AssertType(o, "{ x: number; y: number; }");
AssertType({        ...o,        ...nt && { x: nt }    }, "{ x: number; y: number; }");

        ...o,
AssertType(o, "{ x: number; y: number; }");

        ...nt && { x: nt 
AssertType(nt && { x: nt }, "union");

AssertType(nt, "number");

AssertType({ x: nt }, "{ x: number; }");

AssertType(x, "number");

AssertType(nt, "number");
}
    }
    let o2 = { ...nt && { x: nt }
AssertType(o2, "{ x?: union; }");

AssertType({ ...nt && { x: nt }}, "{ x?: union; }");

AssertType(nt && { x: nt }, "union");

AssertType(nt, "number");

AssertType({ x: nt }, "{ x: number; }");

AssertType(x, "number");

AssertType(nt, "number");
}

AssertType(o, "{ x: number; y: number; }");
    return o;
}
function conditionalSpreadString(st: string): { x: string, y: number } {
    let o = { x: 'hi', y: 17 
AssertType(o, "{ x: string; y: number; }");

AssertType({ x: 'hi', y: 17 }, "{ x: string; y: number; }");

AssertType(x, "string");

AssertType('hi', "string");

AssertType(y, "number");

AssertType(17, "int");
}

    o = {
AssertType(o = {        ...o,        ...st && { x: st }    }, "{ x: string; y: number; }");
AssertType(o, "{ x: string; y: number; }");
AssertType({        ...o,        ...st && { x: st }    }, "{ x: string; y: number; }");

        ...o,
AssertType(o, "{ x: string; y: number; }");

        ...st && { x: st 
AssertType(st && { x: st }, "union");

AssertType(st, "string");

AssertType({ x: st }, "{ x: string; }");

AssertType(x, "string");

AssertType(st, "string");
}
    }
    let o2 = { ...st && { x: st }
AssertType(o2, "{ x?: union; }");

AssertType({ ...st && { x: st }}, "{ x?: union; }");

AssertType(st && { x: st }, "union");

AssertType(st, "string");

AssertType({ x: st }, "{ x: string; }");

AssertType(x, "string");

AssertType(st, "string");
}

AssertType(o, "{ x: string; y: number; }");
    return o;
}

// any results in any
let anything: any;
AssertType(anything, "any");

let spreadAny = { ...anything };
AssertType(spreadAny, "any");
AssertType({ ...anything }, "any");
AssertType(anything, "any");

// methods are not enumerable
class C { p = 1; m() { } }
let c: C = new C()
AssertType(c, "C");
AssertType(new C(), "C");
AssertType(C, "typeof C");

let spreadC: { p: number } = { ...c 
AssertType(spreadC, "{ p: number; }");

AssertType(p, "number");

AssertType({ ...c }, "{ p: number; }");

AssertType(c, "C");
}

// own methods are enumerable
let cplus: { p: number, plus(): void } = { ...c, plus() { 
AssertType(cplus, "{ p: number; plus(): void; }");
AssertType(p, "number");
AssertType(plus, "() => void");
AssertType({ ...c, plus() { return this.p + 1; } }, "{ plus(): any; p: number; }");
AssertType(c, "C");
AssertType(plus, "() => any");
AssertType(this.p + 1, "any");
AssertType(this.p, "any");
AssertType(this, "any");
AssertType(1, "int");
return this.p + 1; } };

cplus.plus();
AssertType(cplus.plus(), "void");
AssertType(cplus.plus, "() => void");

// new field's type conflicting with existing field is OK
let changeTypeAfter: { a: string, b: string } =
AssertType(changeTypeAfter, "{ a: string; b: string; }");
AssertType(a, "string");
AssertType(b, "string");

    { ...o, a: 'wrong type?' 
AssertType({ ...o, a: 'wrong type?' }, "{ a: string; b: string; }");

AssertType(o, "{ a: number; b: string; }");

AssertType(a, "string");

AssertType('wrong type?', "string");
}

let changeTypeBoth: { a: string, b: number } =
AssertType(changeTypeBoth, "{ a: string; b: number; }");
AssertType(a, "string");
AssertType(b, "number");

    { ...o, ...swap };
AssertType({ ...o, ...swap }, "{ a: string; b: number; }");
AssertType(o, "{ a: number; b: string; }");
AssertType(swap, "{ a: string; b: number; }");

// optional
function container(
    definiteBoolean: { sn: boolean },
    definiteString: { sn: string },
    optionalString: { sn?: string },
    optionalNumber: { sn?: number }) {
    let optionalUnionStops: { sn: string | number | boolean } = { ...definiteBoolean, ...definiteString, ...optionalNumber };
AssertType(optionalUnionStops, "{ sn: union; }");
AssertType(sn, "union");
AssertType({ ...definiteBoolean, ...definiteString, ...optionalNumber }, "{ sn: union; }");
AssertType(definiteBoolean, "{ sn: boolean; }");
AssertType(definiteString, "{ sn: string; }");
AssertType(optionalNumber, "{ sn?: union; }");

    let optionalUnionDuplicates: { sn: string | number } = { ...definiteBoolean, ...definiteString, ...optionalString, ...optionalNumber };
AssertType(optionalUnionDuplicates, "{ sn: union; }");
AssertType(sn, "union");
AssertType({ ...definiteBoolean, ...definiteString, ...optionalString, ...optionalNumber }, "{ sn: union; }");
AssertType(definiteBoolean, "{ sn: boolean; }");
AssertType(definiteString, "{ sn: string; }");
AssertType(optionalString, "{ sn?: union; }");
AssertType(optionalNumber, "{ sn?: union; }");

    let allOptional: { sn?: string | number } = { ...optionalString, ...optionalNumber };
AssertType(allOptional, "{ sn?: union; }");
AssertType(sn, "union");
AssertType({ ...optionalString, ...optionalNumber }, "{ sn?: union; }");
AssertType(optionalString, "{ sn?: union; }");
AssertType(optionalNumber, "{ sn?: union; }");

    // computed property
    let computedFirst: { a: number, b: string, "before everything": number } =
AssertType(computedFirst, "{ a: number; b: string; "before everything": number; }");
AssertType(a, "number");
AssertType(b, "string");
AssertType("before everything", "number");

        { ['before everything']: 12, ...o, b: 'yes' 
AssertType({ ['before everything']: 12, ...o, b: 'yes' }, "{ b: string; a: number; "before everything": number; }");

AssertType(['before everything'], "number");

AssertType('before everything', "string");

AssertType(12, "int");

AssertType(o, "{ a: number; b: string; }");

AssertType(b, "string");

AssertType('yes', "string");
}

    let computedAfter: { a: number, b: string, "at the end": number } =
AssertType(computedAfter, "{ a: number; b: string; "at the end": number; }");
AssertType(a, "number");
AssertType(b, "string");
AssertType("at the end", "number");

        { ...o, b: 'yeah', ['at the end']: 14 
AssertType({ ...o, b: 'yeah', ['at the end']: 14 }, "{ b: string; "at the end": number; a: number; }");

AssertType(o, "{ a: number; b: string; }");

AssertType(b, "string");

AssertType('yeah', "string");

AssertType(['at the end'], "number");

AssertType('at the end', "string");

AssertType(14, "int");
}
}
// shortcut syntax
let a = 12;
AssertType(a, "number");
AssertType(12, "int");

let shortCutted: { a: number, b: string } = { ...o, a 
AssertType(shortCutted, "{ a: number; b: string; }");

AssertType(a, "number");

AssertType(b, "string");

AssertType({ ...o, a }, "{ a: number; b: string; }");

AssertType(o, "{ a: number; b: string; }");

AssertType(a, "number");
}

// non primitive
let spreadNonPrimitive = { ...<object>{}};
AssertType(spreadNonPrimitive, "{}");
AssertType({ ...<object>{}}, "{}");
AssertType(<object>{}, "object");
AssertType({}, "{}");

// generic spreads

function f<T, U>(t: T, u: U) {
AssertType({ ...t, ...u, id: 'id' }, "T & U & { id: string; }");
AssertType(t, "T");
AssertType(u, "U");
AssertType(id, "string");
AssertType('id', "string");
    return { ...t, ...u, id: 'id' };
}

let exclusive: { id: string, a: number, b: string, c: string, d: boolean } =
AssertType(exclusive, "{ id: string; a: number; b: string; c: string; d: boolean; }");
AssertType(id, "string");
AssertType(a, "number");
AssertType(b, "string");
AssertType(c, "string");
AssertType(d, "boolean");

    f({ a: 1, b: 'yes' }, { c: 'no', d: false })
AssertType(f({ a: 1, b: 'yes' }, { c: 'no', d: false }), "{ a: number; b: string; } & { c: string; d: boolean; } & { id: string; }");
AssertType(f, "<T, U>(T, U) => T & U & { id: string; }");
AssertType({ a: 1, b: 'yes' }, "{ a: number; b: string; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType(b, "string");
AssertType('yes', "string");
AssertType({ c: 'no', d: false }, "{ c: string; d: false; }");
AssertType(c, "string");
AssertType('no', "string");
AssertType(d, "boolean");
AssertType(false, "boolean");

let overlap: { id: string, a: number, b: string } =
AssertType(overlap, "{ id: string; a: number; b: string; }");
AssertType(id, "string");
AssertType(a, "number");
AssertType(b, "string");

    f({ a: 1 }, { a: 2, b: 'extra' })
AssertType(f({ a: 1 }, { a: 2, b: 'extra' }), "{ a: number; } & { a: number; b: string; } & { id: string; }");
AssertType(f, "<T, U>(T, U) => T & U & { id: string; }");
AssertType({ a: 1 }, "{ a: number; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType({ a: 2, b: 'extra' }, "{ a: number; b: string; }");
AssertType(a, "number");
AssertType(2, "int");
AssertType(b, "string");
AssertType('extra', "string");

let overlapConflict: { id:string, a: string } =
AssertType(overlapConflict, "{ id: string; a: string; }");
AssertType(id, "string");
AssertType(a, "string");

    f({ a: 1 }, { a: 'mismatch' })
AssertType(f({ a: 1 }, { a: 'mismatch' }), "{ a: number; } & { a: string; } & { id: string; }");
AssertType(f, "<T, U>(T, U) => T & U & { id: string; }");
AssertType({ a: 1 }, "{ a: number; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType({ a: 'mismatch' }, "{ a: string; }");
AssertType(a, "string");
AssertType('mismatch', "string");

let overwriteId: { id: string, a: number, c: number, d: string } =
AssertType(overwriteId, "{ id: string; a: number; c: number; d: string; }");
AssertType(id, "string");
AssertType(a, "number");
AssertType(c, "number");
AssertType(d, "string");

    f({ a: 1, id: true }, { c: 1, d: 'no' })
AssertType(f({ a: 1, id: true }, { c: 1, d: 'no' }), "never");
AssertType(f, "<T, U>(T, U) => T & U & { id: string; }");
AssertType({ a: 1, id: true }, "{ a: number; id: true; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType(id, "boolean");
AssertType(true, "boolean");
AssertType({ c: 1, d: 'no' }, "{ c: number; d: string; }");
AssertType(c, "number");
AssertType(1, "int");
AssertType(d, "string");
AssertType('no', "string");

function genericSpread<T, U>(t: T, u: U, v: T | U, w: T | { s: string }, obj: { x: number }) {
    let x01 = { ...t };
AssertType(x01, "T");
AssertType({ ...t }, "T");
AssertType(t, "T");

    let x02 = { ...t, ...t };
AssertType(x02, "T");
AssertType({ ...t, ...t }, "T");
AssertType(t, "T");
AssertType(t, "T");

    let x03 = { ...t, ...u };
AssertType(x03, "T & U");
AssertType({ ...t, ...u }, "T & U");
AssertType(t, "T");
AssertType(u, "U");

    let x04 = { ...u, ...t };
AssertType(x04, "U & T");
AssertType({ ...u, ...t }, "U & T");
AssertType(u, "U");
AssertType(t, "T");

    let x05 = { a: 5, b: 'hi', ...t };
AssertType(x05, "{ a: number; b: string; } & T");
AssertType({ a: 5, b: 'hi', ...t }, "{ a: number; b: string; } & T");
AssertType(a, "number");
AssertType(5, "int");
AssertType(b, "string");
AssertType('hi', "string");
AssertType(t, "T");

    let x06 = { ...t, a: 5, b: 'hi' };
AssertType(x06, "T & { a: number; b: string; }");
AssertType({ ...t, a: 5, b: 'hi' }, "T & { a: number; b: string; }");
AssertType(t, "T");
AssertType(a, "number");
AssertType(5, "int");
AssertType(b, "string");
AssertType('hi', "string");

    let x07 = { a: 5, b: 'hi', ...t, c: true, ...obj };
AssertType(x07, "{ a: number; b: string; } & T & { x: number; c: boolean; }");
AssertType({ a: 5, b: 'hi', ...t, c: true, ...obj }, "{ a: number; b: string; } & T & { x: number; c: boolean; }");
AssertType(a, "number");
AssertType(5, "int");
AssertType(b, "string");
AssertType('hi', "string");
AssertType(t, "T");
AssertType(c, "boolean");
AssertType(true, "boolean");
AssertType(obj, "{ x: number; }");

    let x09 = { a: 5, ...t, b: 'hi', c: true, ...obj };
AssertType(x09, "{ a: number; } & T & { x: number; b: string; c: boolean; }");
AssertType({ a: 5, ...t, b: 'hi', c: true, ...obj }, "{ a: number; } & T & { x: number; b: string; c: boolean; }");
AssertType(a, "number");
AssertType(5, "int");
AssertType(t, "T");
AssertType(b, "string");
AssertType('hi', "string");
AssertType(c, "boolean");
AssertType(true, "boolean");
AssertType(obj, "{ x: number; }");

    let x10 = { a: 5, ...t, b: 'hi', ...u, ...obj };
AssertType(x10, "{ a: number; } & T & { b: string; } & U & { x: number; }");
AssertType({ a: 5, ...t, b: 'hi', ...u, ...obj }, "{ a: number; } & T & { b: string; } & U & { x: number; }");
AssertType(a, "number");
AssertType(5, "int");
AssertType(t, "T");
AssertType(b, "string");
AssertType('hi', "string");
AssertType(u, "U");
AssertType(obj, "{ x: number; }");

    let x11 = { ...v };
AssertType(x11, "union");
AssertType({ ...v }, "union");
AssertType(v, "union");

    let x12 = { ...v, ...obj };
AssertType(x12, "union");
AssertType({ ...v, ...obj }, "union");
AssertType(v, "union");
AssertType(obj, "{ x: number; }");

    let x13 = { ...w };
AssertType(x13, "union");
AssertType({ ...w }, "union");
AssertType(w, "union");

    let x14 = { ...w, ...obj };
AssertType(x14, "union");
AssertType({ ...w, ...obj }, "union");
AssertType(w, "union");
AssertType(obj, "{ x: number; }");

    let x15 = { ...t, ...v };
AssertType(x15, "union");
AssertType({ ...t, ...v }, "union");
AssertType(t, "T");
AssertType(v, "union");

    let x16 = { ...t, ...w };
AssertType(x16, "union");
AssertType({ ...t, ...w }, "union");
AssertType(t, "T");
AssertType(w, "union");

    let x17 = { ...t, ...w, ...obj };
AssertType(x17, "union");
AssertType({ ...t, ...w, ...obj }, "union");
AssertType(t, "T");
AssertType(w, "union");
AssertType(obj, "{ x: number; }");

    let x18 = { ...t, ...v, ...w };
AssertType(x18, "union");
AssertType({ ...t, ...v, ...w }, "union");
AssertType(t, "T");
AssertType(v, "union");
AssertType(w, "union");
}


