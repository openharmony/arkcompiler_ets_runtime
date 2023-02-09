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

// === tests/cases/conformance/types/typeRelationships/typeInference/unionAndIntersectionInference1.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #2264

interface Y { 'i am a very certain type': Y }
let y: Y = <Y>undefined;
AssertType(y, "Y");
AssertType(<Y>undefined, "Y");
AssertType(undefined, "undefined");

function destructure<a, r>(
    something: a | Y,
    haveValue: (value: a) => r,
    haveY: (value: Y) => r
): r {
AssertType(something === y ? haveY(y) : haveValue(<a>something), "r");
AssertType(something === y, "boolean");
AssertType(something, "union");
AssertType(y, "Y");
AssertType(haveY(y), "r");
AssertType(haveY, "(Y) => r");
AssertType(y, "Y");
AssertType(haveValue(<a>something), "r");
AssertType(haveValue, "(a) => r");
AssertType(<a>something, "a");
AssertType(something, "union");
    return something === y ? haveY(y) : haveValue(<a>something);
}

let value = Math.random() > 0.5 ? 'hey!' : <Y>undefined;
AssertType(value, "union");
AssertType(Math.random() > 0.5 ? 'hey!' : <Y>undefined, "union");
AssertType(Math.random() > 0.5, "boolean");
AssertType(Math.random(), "number");
AssertType(Math.random, "() => number");
AssertType(0.5, "double");
AssertType('hey!', "string");
AssertType(<Y>undefined, "Y");
AssertType(undefined, "undefined");

let result = destructure(value, text => 'string', y => 'other one'); // text: string, y: Y
AssertType(result, "string");
AssertType(destructure(value, text => 'string', y => 'other one'), "string");
AssertType(destructure, "<a, r>(union, (a) => r, (Y) => r) => r");
AssertType(value, "union");
AssertType(text => 'string', "(string) => string");
AssertType(text, "string");
AssertType('string', "string");
AssertType(y => 'other one', "(Y) => string");
AssertType(y, "Y");
AssertType('other one', "string");

// Repro from #4212

function isVoid<a>(value: void | a): value is void {
AssertType(undefined, "undefined");
    return undefined;
}

function isNonVoid<a>(value: void | a) : value is a {
AssertType(undefined, "undefined");
    return undefined;
}

function foo1<a>(value: void|a): void {
    if (isVoid(value)) {
AssertType(isVoid(value), "boolean");
AssertType(isVoid, "<a>(union) => value is void");
AssertType(value, "union");

        value; // value is void
AssertType(value, "void");

    } else {
        value; // value is a
AssertType(value, "a");
    }
}

function baz1<a>(value: void|a): void {
      if (isNonVoid(value)) {
AssertType(isNonVoid(value), "boolean");
AssertType(isNonVoid, "<a>(union) => value is a");
AssertType(value, "union");

          value; // value is a
AssertType(value, "a");

      } else {
          value; // value is void
AssertType(value, "void");
      }
}

// Repro from #5417

type Maybe<T> = T | void;

function get<U>(x: U | void): U {
AssertType(null, "null");
   return null; // just an example
}

let foo: Maybe<string>;
AssertType(foo, "Maybe<string>");

get(foo).toUpperCase(); // Ok
AssertType(get(foo).toUpperCase(), "string");
AssertType(get(foo).toUpperCase, "() => string");

// Repro from #5456

interface Man {
    walks: boolean;
}

interface Bear {
    roars: boolean;
}

interface Pig {
    oinks: boolean;
}

declare function pigify<T>(y: T & Bear): T & Pig;
declare let mbp: Man & Bear;
AssertType(mbp, "Man & Bear");

pigify(mbp).oinks; // OK, mbp is treated as Pig
AssertType(pigify(mbp).oinks, "boolean");

pigify(mbp).walks; // Ok, mbp is treated as Man
AssertType(pigify(mbp).walks, "boolean");

// Repros from #29815

interface ITest {
  name: 'test'
}

const createTestAsync = (): Promise<ITest> => Promise.resolve().then(() => ({ name: 'test' }))
AssertType(createTestAsync, "() => Promise<ITest>");
AssertType((): Promise<ITest> => Promise.resolve().then(() => ({ name: 'test' })), "() => Promise<ITest>");
AssertType(Promise.resolve().then(() => ({ name: 'test' })), "Promise<union>");
AssertType(Promise.resolve().then, "<TResult1 = void, TResult2 = never>(?(void) => union, ?(any) => union) => Promise<union>");
AssertType(() => ({ name: 'test' }), "() => { name: "test"; }");
AssertType(({ name: 'test' }), "{ name: "test"; }");
AssertType({ name: 'test' }, "{ name: "test"; }");
AssertType(name, "string");
AssertType('test', "string");

const createTest = (): ITest => {
AssertType(createTest, "() => ITest");
AssertType((): ITest => {  return { name: 'test' }}, "() => ITest");

AssertType({ name: 'test' }, "{ name: "test"; }");
  return { name: 'test' 

AssertType(name, "string");

AssertType('test', "string");
}
}

declare function f1<T, U>(x: T | U): T | U;
declare function f2<T, U>(x: T, y: U): T | U;

let x1: string = f1('a');
AssertType(x1, "string");
AssertType(f1('a'), "string");
AssertType(f1, "<T, U>(union) => union");
AssertType('a', "string");

let x2: string = f2('a', 'b');
AssertType(x2, "string");
AssertType(f2('a', 'b'), "union");
AssertType(f2, "<T, U>(T, U) => union");
AssertType('a', "string");
AssertType('b', "string");

// Repro from #30442

const func = <T>() => {};
AssertType(func, "<T>() => void");
AssertType(<T>() => {}, "<T>() => void");

const assign = <T, U>(a: T, b: U) => Object.assign(a, b);
AssertType(assign, "<T, U>(T, U) => T & U");
AssertType(<T, U>(a: T, b: U) => Object.assign(a, b), "<T, U>(T, U) => T & U");
AssertType(a, "T");
AssertType(b, "U");
AssertType(Object.assign(a, b), "T & U");
AssertType(Object.assign, "{ <T extends {}, U>(T, U): T & U; <T extends {}, U, V>(T, U, V): T & U & V; <T extends {}, U, V, W>(T, U, V, W): T & U & V & W; (object, ...any[]): any; }");
AssertType(a, "T");
AssertType(b, "U");

const res: (() => void) & { func: any } = assign(() => {}, { func });
AssertType(res, "(() => void) & { func: any; }");
AssertType(func, "any");
AssertType(assign(() => {}, { func }), "(() => void) & { func: <T>() => void; }");
AssertType(assign, "<T, U>(T, U) => T & U");
AssertType(() => {}, "() => void");
AssertType({ func }, "{ func: <T>() => void; }");
AssertType(func, "<T>() => void");


