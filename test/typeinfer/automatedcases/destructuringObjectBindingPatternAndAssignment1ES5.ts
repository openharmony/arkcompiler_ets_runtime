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

// === tests/cases/conformance/es6/destructuring/destructuringObjectBindingPatternAndAssignment1ES5.ts ===
declare function AssertType(value:any, type:string):void;
// In a destructuring assignment expression, the type of the expression on the right must be assignable to the assignment target on the left.
// An expression of type S is considered assignable to an assignment target V if one of the following is true

// V is an object assignment pattern and, for each assignment property P in V,
//      S is the type Any, or
let { a1 }: any = undefined;
AssertType(a1, "any");
AssertType(undefined, "undefined");

let { a2 }: any = {};
AssertType(a2, "any");
AssertType({}, "{}");

// V is an object assignment pattern and, for each assignment property P in V,
//      S has an apparent property with the property name specified in
//          P of a type that is assignable to the target given in P, or
let { b1, } = { b1:1, };
AssertType(b1, "number");
AssertType({ b1:1, }, "{ b1: number; }");
AssertType(b1, "number");
AssertType(1, "int");

let { b2: { b21 } = { b21: "string" }  } = { b2: { b21: "world" } };
AssertType(b2, "any");
AssertType(b21, "string");
AssertType({ b21: "string" }, "{ b21: string; }");
AssertType(b21, "string");
AssertType("string", "string");
AssertType({ b2: { b21: "world" } }, "{ b2?: { b21: string; }; }");
AssertType(b2, "{ b21: string; }");
AssertType({ b21: "world" }, "{ b21: string; }");
AssertType(b21, "string");
AssertType("world", "string");

let {1: b3} = { 1: "string" };
AssertType(b3, "string");
AssertType({ 1: "string" }, "{ 1: string; }");
AssertType(1, "string");
AssertType("string", "string");

let {b4 = 1}: any = { b4: 100000 };
AssertType(b4, "any");
AssertType(1, "int");
AssertType({ b4: 100000 }, "{ b4: number; }");
AssertType(b4, "number");
AssertType(100000, "int");

let {b5: { b52 }  } = { b5: { b52 } };
AssertType(b5, "any");
AssertType(b52, "any");
AssertType({ b5: { b52 } }, "{ b5: { b52: any; }; }");
AssertType(b5, "{ b52: any; }");
AssertType({ b52 }, "{ b52: any; }");
AssertType(b52, "any");

// V is an object assignment pattern and, for each assignment property P in V,
//      P specifies a numeric property name and S has a numeric index signature
//          of a type that is assignable to the target given in P, or

interface F {
    [idx: number]: boolean;
}

function foo(): F {
AssertType({        1: true    }, "{ 1: true; }");
    return {

        1: true
AssertType(1, "boolean");
AssertType(true, "boolean");

    };
}

function bar(): F {
AssertType({        2: true    }, "{ 2: true; }");
    return {

        2: true
AssertType(2, "boolean");
AssertType(true, "boolean");

    };
}
let {1: c0} = foo();
AssertType(c0, "boolean");
AssertType(foo(), "F");
AssertType(foo, "() => F");

let {1: c1} = bar();
AssertType(c1, "boolean");
AssertType(bar(), "F");
AssertType(bar, "() => F");

// V is an object assignment pattern and, for each assignment property P in V,
//      S has a string index signature of a type that is assignable to the target given in P

interface F1 {
    [str: string]: number;
}

function foo1(): F1 {
AssertType({        "prop1": 2    }, "{ prop1: number; }");
    return {

        "prop1": 2
AssertType("prop1", "number");
AssertType(2, "int");
    }
}

let {"prop1": d1} = foo1();
AssertType(d1, "number");
AssertType(foo1(), "F1");
AssertType(foo1, "() => F1");

let {"prop2": d1} = foo1();
AssertType(d1, "number");
AssertType(foo1(), "F1");
AssertType(foo1, "() => F1");


