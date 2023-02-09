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

// === tests/cases/compiler/narrowingByTypeofInSwitch.ts ===
declare function AssertType(value:any, type:string):void;
function assertNever(x: never) {
AssertType(x, "never");
    return x;
}

function assertNumber(x: number) {
AssertType(x, "number");
    return x;
}

function assertBoolean(x: boolean) {
AssertType(x, "boolean");
    return x;
}

function assertString(x: string) {
AssertType(x, "string");
    return x;
}

function assertSymbol(x: symbol) {
AssertType(x, "symbol");
    return x;
}

function assertFunction(x: Function) {
AssertType(x, "Function");
    return x;
}

function assertObject(x: object) {
AssertType(x, "object");
    return x;
}

function assertObjectOrNull(x: object | null) {
AssertType(x, "union");
    return x;
}

function assertUndefined(x: undefined) {
AssertType(x, "undefined");
    return x;
}

function assertAll(x: Basic) {
AssertType(x, "Basic");
    return x;
}

function assertStringOrNumber(x: string | number) {
AssertType(x, "union");
    return x;
}

function assertBooleanOrObject(x: boolean | object) {
AssertType(x, "union");
    return x;
}

type Basic = number | boolean | string | symbol | object | Function | undefined;

function testUnion(x: Basic) {
    switch (typeof x) {
AssertType(typeof x, "union");
AssertType(x, "Basic");

        case 'number': assertNumber(x); 
AssertType('number', "string");
AssertType(assertNumber(x), "number");
AssertType(assertNumber, "(number) => number");
AssertType(x, "number");
return;

        case 'boolean': assertBoolean(x); 
AssertType('boolean', "string");
AssertType(assertBoolean(x), "boolean");
AssertType(assertBoolean, "(boolean) => boolean");
AssertType(x, "boolean");
return;

        case 'function': assertFunction(x); 
AssertType('function', "string");
AssertType(assertFunction(x), "Function");
AssertType(assertFunction, "(Function) => Function");
AssertType(x, "Function");
return;

        case 'symbol': assertSymbol(x); 
AssertType('symbol', "string");
AssertType(assertSymbol(x), "symbol");
AssertType(assertSymbol, "(symbol) => symbol");
AssertType(x, "symbol");
return;

        case 'object': assertObject(x); 
AssertType('object', "string");
AssertType(assertObject(x), "object");
AssertType(assertObject, "(object) => object");
AssertType(x, "object");
return;

        case 'string': assertString(x); 
AssertType('string', "string");
AssertType(assertString(x), "string");
AssertType(assertString, "(string) => string");
AssertType(x, "string");
return;

        case 'undefined': assertUndefined(x); 
AssertType('undefined', "string");
AssertType(assertUndefined(x), "undefined");
AssertType(assertUndefined, "(undefined) => undefined");
AssertType(x, "undefined");
return;
    }
    assertNever(x);
AssertType(assertNever(x), "never");
AssertType(assertNever, "(never) => never");
AssertType(x, "never");
}

function testExtendsUnion<T extends Basic>(x: T) {
    switch (typeof x) {
AssertType(typeof x, "union");
AssertType(x, "T");

        case 'number': assertNumber(x); 
AssertType('number', "string");
AssertType(assertNumber(x), "number");
AssertType(assertNumber, "(number) => number");
AssertType(x, "number");
return;

        case 'boolean': assertBoolean(x); 
AssertType('boolean', "string");
AssertType(assertBoolean(x), "boolean");
AssertType(assertBoolean, "(boolean) => boolean");
AssertType(x, "boolean");
return;

        case 'function': assertAll(x); 
AssertType('function', "string");
AssertType(assertAll(x), "Basic");
AssertType(assertAll, "(Basic) => Basic");
AssertType(x, "Function");
return;

        case 'symbol': assertSymbol(x); 
AssertType('symbol', "string");
AssertType(assertSymbol(x), "symbol");
AssertType(assertSymbol, "(symbol) => symbol");
AssertType(x, "symbol");
return;

        case 'object': assertAll(x); 
AssertType('object', "string");
AssertType(assertAll(x), "Basic");
AssertType(assertAll, "(Basic) => Basic");
AssertType(x, "object");
return;

        case 'string': assertString(x); 
AssertType('string', "string");
AssertType(assertString(x), "string");
AssertType(assertString, "(string) => string");
AssertType(x, "string");
return;

        case 'undefined': assertUndefined(x); 
AssertType('undefined', "string");
AssertType(assertUndefined(x), "undefined");
AssertType(assertUndefined, "(undefined) => undefined");
AssertType(x, "undefined");
return;
    }
    assertAll(x);
AssertType(assertAll(x), "Basic");
AssertType(assertAll, "(Basic) => Basic");
AssertType(x, "never");
}

function testAny(x: any) {
    switch (typeof x) {
AssertType(typeof x, "union");
AssertType(x, "any");

        case 'number': assertNumber(x); 
AssertType('number', "string");
AssertType(assertNumber(x), "number");
AssertType(assertNumber, "(number) => number");
AssertType(x, "number");
return;

        case 'boolean': assertBoolean(x); 
AssertType('boolean', "string");
AssertType(assertBoolean(x), "boolean");
AssertType(assertBoolean, "(boolean) => boolean");
AssertType(x, "boolean");
return;

        case 'function': assertFunction(x); 
AssertType('function', "string");
AssertType(assertFunction(x), "Function");
AssertType(assertFunction, "(Function) => Function");
AssertType(x, "any");
return;

        case 'symbol': assertSymbol(x); 
AssertType('symbol', "string");
AssertType(assertSymbol(x), "symbol");
AssertType(assertSymbol, "(symbol) => symbol");
AssertType(x, "symbol");
return;

        case 'object': assertObject(x); 
AssertType('object', "string");
AssertType(assertObject(x), "object");
AssertType(assertObject, "(object) => object");
AssertType(x, "any");
return;

        case 'string': assertString(x); 
AssertType('string', "string");
AssertType(assertString(x), "string");
AssertType(assertString, "(string) => string");
AssertType(x, "string");
return;

        case 'undefined': assertUndefined(x); 
AssertType('undefined', "string");
AssertType(assertUndefined(x), "undefined");
AssertType(assertUndefined, "(undefined) => undefined");
AssertType(x, "undefined");
return;
    }
    assertAll(x); // is any
AssertType(assertAll(x), "Basic");
AssertType(assertAll, "(Basic) => Basic");
AssertType(x, "any");
}

function a1(x: string | object | undefined) {
AssertType(x, "union");
    return x;
}

function testUnionExplicitDefault(x: Basic) {
    switch (typeof x) {
AssertType(typeof x, "union");
AssertType(x, "Basic");

        case 'number': assertNumber(x); 
AssertType('number', "string");
AssertType(assertNumber(x), "number");
AssertType(assertNumber, "(number) => number");
AssertType(x, "number");
return;

        case 'boolean': assertBoolean(x); 
AssertType('boolean', "string");
AssertType(assertBoolean(x), "boolean");
AssertType(assertBoolean, "(boolean) => boolean");
AssertType(x, "boolean");
return;

        case 'function': assertFunction(x); 
AssertType('function', "string");
AssertType(assertFunction(x), "Function");
AssertType(assertFunction, "(Function) => Function");
AssertType(x, "Function");
return;

        case 'symbol': assertSymbol(x); 
AssertType('symbol', "string");
AssertType(assertSymbol(x), "symbol");
AssertType(assertSymbol, "(symbol) => symbol");
AssertType(x, "symbol");
return;

        default: a1(x); 
AssertType(a1(x), "union");
AssertType(a1, "(union) => union");
AssertType(x, "union");
return;
    }
}

function testUnionImplicitDefault(x: Basic) {
    switch (typeof x) {
AssertType(typeof x, "union");
AssertType(x, "Basic");

        case 'number': assertNumber(x); 
AssertType('number', "string");
AssertType(assertNumber(x), "number");
AssertType(assertNumber, "(number) => number");
AssertType(x, "number");
return;

        case 'boolean': assertBoolean(x); 
AssertType('boolean', "string");
AssertType(assertBoolean(x), "boolean");
AssertType(assertBoolean, "(boolean) => boolean");
AssertType(x, "boolean");
return;

        case 'function': assertFunction(x); 
AssertType('function', "string");
AssertType(assertFunction(x), "Function");
AssertType(assertFunction, "(Function) => Function");
AssertType(x, "Function");
return;

        case 'symbol': assertSymbol(x); 
AssertType('symbol', "string");
AssertType(assertSymbol(x), "symbol");
AssertType(assertSymbol, "(symbol) => symbol");
AssertType(x, "symbol");
return;
    }
AssertType(a1(x), "union");
AssertType(a1, "(union) => union");
AssertType(x, "union");
    return a1(x);
}

function testExtendsExplicitDefault<T extends Basic>(x: T) {
    switch (typeof x) {
AssertType(typeof x, "union");
AssertType(x, "T");

        case 'number': assertNumber(x); 
AssertType('number', "string");
AssertType(assertNumber(x), "number");
AssertType(assertNumber, "(number) => number");
AssertType(x, "number");
return;

        case 'boolean': assertBoolean(x); 
AssertType('boolean', "string");
AssertType(assertBoolean(x), "boolean");
AssertType(assertBoolean, "(boolean) => boolean");
AssertType(x, "boolean");
return;

        case 'function': assertAll(x); 
AssertType('function', "string");
AssertType(assertAll(x), "Basic");
AssertType(assertAll, "(Basic) => Basic");
AssertType(x, "Function");
return;

        case 'symbol': assertSymbol(x); 
AssertType('symbol', "string");
AssertType(assertSymbol(x), "symbol");
AssertType(assertSymbol, "(symbol) => symbol");
AssertType(x, "symbol");
return;

        default: assertAll(x); 
AssertType(assertAll(x), "Basic");
AssertType(assertAll, "(Basic) => Basic");
AssertType(x, "union");
return;

    }
}

function testExtendsImplicitDefault<T extends Basic>(x: T) {
    switch (typeof x) {
AssertType(typeof x, "union");
AssertType(x, "T");

        case 'number': assertNumber(x); 
AssertType('number', "string");
AssertType(assertNumber(x), "number");
AssertType(assertNumber, "(number) => number");
AssertType(x, "number");
return;

        case 'boolean': assertBoolean(x); 
AssertType('boolean', "string");
AssertType(assertBoolean(x), "boolean");
AssertType(assertBoolean, "(boolean) => boolean");
AssertType(x, "boolean");
return;

        case 'function': assertAll(x); 
AssertType('function', "string");
AssertType(assertAll(x), "Basic");
AssertType(assertAll, "(Basic) => Basic");
AssertType(x, "Function");
return;

        case 'symbol': assertSymbol(x); 
AssertType('symbol', "string");
AssertType(assertSymbol(x), "symbol");
AssertType(assertSymbol, "(symbol) => symbol");
AssertType(x, "symbol");
return;
    }
AssertType(assertAll(x), "Basic");
AssertType(assertAll, "(Basic) => Basic");
AssertType(x, "union");
    return assertAll(x);
}

type L = (x: number) => string;
type R = { x: string, y: number }

function exhaustiveChecks(x: number | string | L | R): string {
    switch (typeof x) {
AssertType(typeof x, "union");
AssertType(x, "union");

        case 'number': 
AssertType('number', "string");
AssertType(x.toString(2), "string");
AssertType(x.toString, "(?union) => string");
AssertType(2, "int");
return x.toString(2);

        case 'string': 
AssertType('string', "string");
AssertType(x, "string");
return x;

        case 'function': 
AssertType('function', "string");
AssertType(x(42), "string");
AssertType(x, "L");
AssertType(42, "int");
return x(42);

        case 'object': 
AssertType('object', "string");
AssertType(x.x, "string");
return x.x;
    }
}

function exhaustiveChecksGenerics<T extends L | R | number | string>(x: T): string {
    switch (typeof x) {
AssertType(typeof x, "union");
AssertType(x, "T");

        case 'number': 
AssertType('number', "string");
AssertType(x.toString(2), "string");
AssertType(x.toString, "(?union) => string");
AssertType(2, "int");
return x.toString(2);

        case 'string': 
AssertType('string', "string");
AssertType(x, "string");
return x;

        case 'function': 
AssertType('function', "string");
AssertType((x as L)(42), "string");
AssertType((x as L), "L");
AssertType(x as L, "L");
AssertType(x, "L");
AssertType(42, "int");
return (x as L)(42); // Can't narrow generic

        case 'object': 
AssertType('object', "string");
AssertType((x as R).x, "string");
AssertType((x as R), "R");
AssertType(x as R, "R");
AssertType(x, "R");
return (x as R).x;            // Can't narrow generic
    }
}

function multipleGeneric<X extends L, Y extends R>(xy: X | Y): [X, string] | [Y, number] {
    switch (typeof xy) {
AssertType(typeof xy, "union");
AssertType(xy, "union");

        case 'function': 
AssertType('function', "string");
AssertType([xy, xy(42)], "[X, string]");
AssertType(xy, "X");
AssertType(xy(42), "string");
AssertType(xy, "X");
AssertType(42, "int");
return [xy, xy(42)];

        case 'object': 
AssertType('object', "string");
AssertType([xy, xy.y], "[Y, number]");
AssertType(xy, "Y");
AssertType(xy.y, "number");
return [xy, xy.y];

        default: 
AssertType(assertNever(xy), "never");
AssertType(assertNever, "(never) => never");
AssertType(xy, "never");
return assertNever(xy);
    }
}

function multipleGenericFuse<X extends L | number, Y extends R | number>(xy: X | Y): [X, number] | [Y, string] | [(X | Y)] {
    switch (typeof xy) {
AssertType(typeof xy, "union");
AssertType(xy, "union");

        case 'function': 
AssertType('function', "string");
AssertType([xy, 1], "[X & Function, 1]");
AssertType(xy, "X & Function");
AssertType(1, "int");
return [xy, 1];

        case 'object': 
AssertType('object', "string");
AssertType([xy, 'two'], "[Y & object, string]");
AssertType(xy, "Y & object");
AssertType('two', "string");
return [xy, 'two'];

        case 'number': 
AssertType('number', "string");
AssertType([xy], "[union]");
AssertType(xy, "union");
return [xy]
    }
}

function multipleGenericExhaustive<X extends L, Y extends R>(xy: X | Y): [X, string] | [Y, number] {
    switch (typeof xy) {
AssertType(typeof xy, "union");
AssertType(xy, "union");

        case 'object': 
AssertType('object', "string");
AssertType([xy, xy.y], "[Y, number]");
AssertType(xy, "Y");
AssertType(xy.y, "number");
return [xy, xy.y];

        case 'function': 
AssertType('function', "string");
AssertType([xy, xy(42)], "[X, string]");
AssertType(xy, "X");
AssertType(xy(42), "string");
AssertType(xy, "X");
AssertType(42, "int");
return [xy, xy(42)];
    }
}

function switchOrdering(x: string | number | boolean) {
    switch (typeof x) {
AssertType(typeof x, "union");
AssertType(x, "union");

        case 'string': 
AssertType('string', "string");
AssertType(assertString(x), "string");
AssertType(assertString, "(string) => string");
AssertType(x, "string");
return assertString(x);

        case 'number': 
AssertType('number', "string");
AssertType(assertNumber(x), "number");
AssertType(assertNumber, "(number) => number");
AssertType(x, "number");
return assertNumber(x);

        case 'boolean': 
AssertType('boolean', "string");
AssertType(assertBoolean(x), "boolean");
AssertType(assertBoolean, "(boolean) => boolean");
AssertType(x, "boolean");
return assertBoolean(x);

        case 'number': 
AssertType('number', "string");
AssertType(assertNever(x), "never");
AssertType(assertNever, "(never) => never");
AssertType(x, "never");
return assertNever(x);
    }
}

function switchOrderingWithDefault(x: string | number | boolean) {
    function local(y: string | number | boolean) {
AssertType(local, "(union) => union");
AssertType(y, "union");

AssertType(x, "union");
        return x;
    }
    switch (typeof x) {
AssertType(typeof x, "union");
AssertType(x, "union");

        case 'string':
AssertType('string', "string");

        case 'number':
AssertType('number', "string");

        default: 
AssertType(local(x), "union");
AssertType(local, "(union) => union");
AssertType(x, "union");
return local(x)

        case 'string': 
AssertType('string', "string");
AssertType(assertNever(x), "never");
AssertType(assertNever, "(never) => never");
AssertType(x, "never");
return assertNever(x);

        case 'number': 
AssertType('number', "string");
AssertType(assertNever(x), "never");
AssertType(assertNever, "(never) => never");
AssertType(x, "never");
return assertNever(x);
    }
}

function fallThroughTest(x: string | number | boolean | object) {
    switch (typeof x) {
AssertType(typeof x, "union");
AssertType(x, "union");

        case 'number':
AssertType('number', "string");

            assertNumber(x)
AssertType(assertNumber(x), "number");
AssertType(assertNumber, "(number) => number");
AssertType(x, "number");

        case 'string':
AssertType('string', "string");

            assertStringOrNumber(x)
AssertType(assertStringOrNumber(x), "union");
AssertType(assertStringOrNumber, "(union) => union");
AssertType(x, "union");

            break;
        default:
            assertObject(x);
AssertType(assertObject(x), "object");
AssertType(assertObject, "(object) => object");
AssertType(x, "object");

        case 'number':
AssertType('number', "string");

        case 'boolean':
AssertType('boolean', "string");

            assertBooleanOrObject(x);
AssertType(assertBooleanOrObject(x), "union");
AssertType(assertBooleanOrObject, "(union) => union");
AssertType(x, "union");

            break;
    }
}

function unknownNarrowing(x: unknown) {
    switch (typeof x) {
AssertType(typeof x, "union");
AssertType(x, "unknown");

        case 'number': assertNumber(x); 
AssertType('number', "string");
AssertType(assertNumber(x), "number");
AssertType(assertNumber, "(number) => number");
AssertType(x, "number");
return;

        case 'boolean': assertBoolean(x); 
AssertType('boolean', "string");
AssertType(assertBoolean(x), "boolean");
AssertType(assertBoolean, "(boolean) => boolean");
AssertType(x, "boolean");
return;

        case 'function': assertFunction(x); 
AssertType('function', "string");
AssertType(assertFunction(x), "Function");
AssertType(assertFunction, "(Function) => Function");
AssertType(x, "Function");
return;

        case 'symbol': assertSymbol(x); 
AssertType('symbol', "string");
AssertType(assertSymbol(x), "symbol");
AssertType(assertSymbol, "(symbol) => symbol");
AssertType(x, "symbol");
return;

        case 'object': assertObjectOrNull(x); 
AssertType('object', "string");
AssertType(assertObjectOrNull(x), "union");
AssertType(assertObjectOrNull, "(union) => union");
AssertType(x, "union");
return;

        case 'string': assertString(x); 
AssertType('string', "string");
AssertType(assertString(x), "string");
AssertType(assertString, "(string) => string");
AssertType(x, "string");
return;

        case 'undefined': assertUndefined(x); 
AssertType('undefined', "string");
AssertType(assertUndefined(x), "undefined");
AssertType(assertUndefined, "(undefined) => undefined");
AssertType(x, "undefined");
return;
    }
}

function keyofNarrowing<S extends { [K in keyof S]: string }>(k: keyof S) {
    function assertKeyofS(k1: keyof S) { 
AssertType(assertKeyofS, "(keyof S) => void");

AssertType(k1, "keyof S");
}

    switch (typeof k) {
AssertType(typeof k, "union");
AssertType(k, "keyof S");

        case 'number': assertNumber(k); assertKeyofS(k); 
AssertType('number', "string");
AssertType(assertNumber(k), "number");
AssertType(assertNumber, "(number) => number");
AssertType(k, "number");
AssertType(assertKeyofS(k), "void");
AssertType(assertKeyofS, "(keyof S) => void");
AssertType(k, "keyof S & number");
return;

        case 'symbol': assertSymbol(k); assertKeyofS(k); 
AssertType('symbol', "string");
AssertType(assertSymbol(k), "symbol");
AssertType(assertSymbol, "(symbol) => symbol");
AssertType(k, "symbol");
AssertType(assertKeyofS(k), "void");
AssertType(assertKeyofS, "(keyof S) => void");
AssertType(k, "keyof S & symbol");
return;

        case 'string': assertString(k); assertKeyofS(k); 
AssertType('string', "string");
AssertType(assertString(k), "string");
AssertType(assertString, "(string) => string");
AssertType(k, "string");
AssertType(assertKeyofS(k), "void");
AssertType(assertKeyofS, "(keyof S) => void");
AssertType(k, "keyof S & string");
return;
    }
}

function narrowingNarrows(x: {} | undefined) {
    switch (typeof x) {
AssertType(typeof x, "union");
AssertType(x, "union");

        case 'number': assertNumber(x); 
AssertType('number', "string");
AssertType(assertNumber(x), "number");
AssertType(assertNumber, "(number) => number");
AssertType(x, "number");
return;

        case 'boolean': assertBoolean(x); 
AssertType('boolean', "string");
AssertType(assertBoolean(x), "boolean");
AssertType(assertBoolean, "(boolean) => boolean");
AssertType(x, "boolean");
return;

        case 'function': assertFunction(x); 
AssertType('function', "string");
AssertType(assertFunction(x), "Function");
AssertType(assertFunction, "(Function) => Function");
AssertType(x, "Function");
return;

        case 'symbol': assertSymbol(x); 
AssertType('symbol', "string");
AssertType(assertSymbol(x), "symbol");
AssertType(assertSymbol, "(symbol) => symbol");
AssertType(x, "symbol");
return;

        case 'object': const _: {} = x; 
AssertType('object', "string");
AssertType(_, "{}");
AssertType(x, "object");
return;

        case 'string': assertString(x); 
AssertType('string', "string");
AssertType(assertString(x), "string");
AssertType(assertString, "(string) => string");
AssertType(x, "string");
return;

        case 'undefined': assertUndefined(x); 
AssertType('undefined', "string");
AssertType(assertUndefined(x), "undefined");
AssertType(assertUndefined, "(undefined) => undefined");
AssertType(x, "undefined");
return;

        case 'number': assertNever(x); 
AssertType('number', "string");
AssertType(assertNever(x), "never");
AssertType(assertNever, "(never) => never");
AssertType(x, "never");
return;

        default: const _y: {} = x; 
AssertType(_y, "{}");
AssertType(x, "{}");
return;
    }
}

function narrowingNarrows2(x: true | 3 | 'hello' | undefined) {
    switch (typeof x) {
AssertType(typeof x, "union");
AssertType(x, "union");

        case 'number': assertNumber(x); 
AssertType('number', "string");
AssertType(assertNumber(x), "number");
AssertType(assertNumber, "(number) => number");
AssertType(x, "int");
return;

        case 'boolean': assertBoolean(x); 
AssertType('boolean', "string");
AssertType(assertBoolean(x), "boolean");
AssertType(assertBoolean, "(boolean) => boolean");
AssertType(x, "boolean");
return;

        case 'function': assertNever(x); 
AssertType('function', "string");
AssertType(assertNever(x), "never");
AssertType(assertNever, "(never) => never");
AssertType(x, "never");
return;

        case 'symbol': assertNever(x); 
AssertType('symbol', "string");
AssertType(assertNever(x), "never");
AssertType(assertNever, "(never) => never");
AssertType(x, "never");
return;

        case 'object': const _: {} = assertNever(x); 
AssertType('object', "string");
AssertType(_, "{}");
AssertType(assertNever(x), "never");
AssertType(assertNever, "(never) => never");
AssertType(x, "never");
return;

        case 'string': assertString(x); 
AssertType('string', "string");
AssertType(assertString(x), "string");
AssertType(assertString, "(string) => string");
AssertType(x, "string");
return;

        case 'undefined': assertUndefined(x); 
AssertType('undefined', "string");
AssertType(assertUndefined(x), "undefined");
AssertType(assertUndefined, "(undefined) => undefined");
AssertType(x, "undefined");
return;

        case 'number': assertNever(x); 
AssertType('number', "string");
AssertType(assertNever(x), "never");
AssertType(assertNever, "(never) => never");
AssertType(x, "never");
return;

        default: const _y: {} = assertNever(x); 
AssertType(_y, "{}");
AssertType(assertNever(x), "never");
AssertType(assertNever, "(never) => never");
AssertType(x, "never");
return;
    }
}

/* Template literals */

function testUnionWithTempalte(x: Basic) {
    switch (typeof x) {
AssertType(typeof x, "union");
AssertType(x, "Basic");

        case `number`: assertNumber(x); 
AssertType(`number`, "string");
AssertType(assertNumber(x), "number");
AssertType(assertNumber, "(number) => number");
AssertType(x, "number");
return;

        case `boolean`: assertBoolean(x); 
AssertType(`boolean`, "string");
AssertType(assertBoolean(x), "boolean");
AssertType(assertBoolean, "(boolean) => boolean");
AssertType(x, "boolean");
return;

        case `function`: assertFunction(x); 
AssertType(`function`, "string");
AssertType(assertFunction(x), "Function");
AssertType(assertFunction, "(Function) => Function");
AssertType(x, "Function");
return;

        case `symbol`: assertSymbol(x); 
AssertType(`symbol`, "string");
AssertType(assertSymbol(x), "symbol");
AssertType(assertSymbol, "(symbol) => symbol");
AssertType(x, "symbol");
return;

        case `object`: assertObject(x); 
AssertType(`object`, "string");
AssertType(assertObject(x), "object");
AssertType(assertObject, "(object) => object");
AssertType(x, "object");
return;

        case `string`: assertString(x); 
AssertType(`string`, "string");
AssertType(assertString(x), "string");
AssertType(assertString, "(string) => string");
AssertType(x, "string");
return;

        case `undefined`: assertUndefined(x); 
AssertType(`undefined`, "string");
AssertType(assertUndefined(x), "undefined");
AssertType(assertUndefined, "(undefined) => undefined");
AssertType(x, "undefined");
return;
    }
    assertNever(x);
AssertType(assertNever(x), "never");
AssertType(assertNever, "(never) => never");
AssertType(x, "never");
}

function fallThroughTestWithTempalte(x: string | number | boolean | object) {
    switch (typeof x) {
AssertType(typeof x, "union");
AssertType(x, "union");

        case `number`:
AssertType(`number`, "string");

            assertNumber(x)
AssertType(assertNumber(x), "number");
AssertType(assertNumber, "(number) => number");
AssertType(x, "number");

        case `string`:
AssertType(`string`, "string");

            assertStringOrNumber(x)
AssertType(assertStringOrNumber(x), "union");
AssertType(assertStringOrNumber, "(union) => union");
AssertType(x, "union");

            break;
        default:
            assertObject(x);
AssertType(assertObject(x), "object");
AssertType(assertObject, "(object) => object");
AssertType(x, "object");

        case `number`:
AssertType(`number`, "string");

        case `boolean`:
AssertType(`boolean`, "string");

            assertBooleanOrObject(x);
AssertType(assertBooleanOrObject(x), "union");
AssertType(assertBooleanOrObject, "(union) => union");
AssertType(x, "union");

            break;
    }
}

function keyofNarrowingWithTemplate<S extends { [K in keyof S]: string }>(k: keyof S) {
    function assertKeyofS(k1: keyof S) { 
AssertType(assertKeyofS, "(keyof S) => void");

AssertType(k1, "keyof S");
}

    switch (typeof k) {
AssertType(typeof k, "union");
AssertType(k, "keyof S");

        case `number`: assertNumber(k); assertKeyofS(k); 
AssertType(`number`, "string");
AssertType(assertNumber(k), "number");
AssertType(assertNumber, "(number) => number");
AssertType(k, "number");
AssertType(assertKeyofS(k), "void");
AssertType(assertKeyofS, "(keyof S) => void");
AssertType(k, "keyof S & number");
return;

        case `symbol`: assertSymbol(k); assertKeyofS(k); 
AssertType(`symbol`, "string");
AssertType(assertSymbol(k), "symbol");
AssertType(assertSymbol, "(symbol) => symbol");
AssertType(k, "symbol");
AssertType(assertKeyofS(k), "void");
AssertType(assertKeyofS, "(keyof S) => void");
AssertType(k, "keyof S & symbol");
return;

        case `string`: assertString(k); assertKeyofS(k); 
AssertType(`string`, "string");
AssertType(assertString(k), "string");
AssertType(assertString, "(string) => string");
AssertType(k, "string");
AssertType(assertKeyofS(k), "void");
AssertType(assertKeyofS, "(keyof S) => void");
AssertType(k, "keyof S & string");
return;
    }
}

/* Both string literals and template literals */

function multipleGenericFuseWithBoth<X extends L | number, Y extends R | number>(xy: X | Y): [X, number] | [Y, string] | [(X | Y)] {
    switch (typeof xy) {
AssertType(typeof xy, "union");
AssertType(xy, "union");

        case `function`: 
AssertType(`function`, "string");
AssertType([xy, 1], "[X & Function, 1]");
AssertType(xy, "X & Function");
AssertType(1, "int");
return [xy, 1];

        case 'object': 
AssertType('object', "string");
AssertType([xy, 'two'], "[Y & object, string]");
AssertType(xy, "Y & object");
AssertType('two', "string");
return [xy, 'two'];

        case `number`: 
AssertType(`number`, "string");
AssertType([xy], "[union]");
AssertType(xy, "union");
return [xy]
    }
}


