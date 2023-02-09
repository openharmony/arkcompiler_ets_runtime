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

// === tests/cases/conformance/expressions/newOperator/newOperatorConformance.ts ===
declare function AssertType(value:any, type:string):void;
class C0 {

}
class C1 {
    constructor(n: number, s: string) { }
}

class T<T> {
    constructor(n?: T) { }
}

let anyCtor: {
AssertType(anyCtor, "new () => any");

    new (): any;
};

let anyCtor1: {
AssertType(anyCtor1, "new (any) => any");

    new (n): any;
AssertType(n, "any");

};

interface nestedCtor {
    new (): nestedCtor;
}
let nestedCtor: nestedCtor;
AssertType(nestedCtor, "nestedCtor");

// Construct expression with no parentheses for construct signature with 0 parameters
let a = new C0;
AssertType(a, "C0");
AssertType(new C0, "C0");
AssertType(C0, "typeof C0");

let a: C0;
AssertType(a, "C0");


// Generic construct expression with no parentheses
let c1 = new T;
AssertType(c1, "T<unknown>");
AssertType(new T, "T<unknown>");
AssertType(T, "typeof T");

let c1: T<{}>;
AssertType(c1, "T<unknown>");

// Construct expression where constructor is of type 'any' with no parentheses
let d = new anyCtor;
AssertType(d, "any");
AssertType(new anyCtor, "any");
AssertType(anyCtor, "new () => any");

let d: any;
AssertType(d, "any");

// Construct expression where constructor is of type 'any' with > 1 arg
let d = new anyCtor1(undefined);
AssertType(d, "any");
AssertType(new anyCtor1(undefined), "any");
AssertType(anyCtor1, "new (any) => any");
AssertType(undefined, "undefined");

// Construct expression of type where apparent type has a construct signature with 0 arguments
function newFn1<T extends { new (): number }>(s: T) {
    let p = new s;
AssertType(p, "number");
AssertType(new s, "number");
AssertType(s, "T");

    let p: number;
AssertType(p, "number");
}

// Construct expression of type where apparent type has a construct signature with 1 arguments
function newFn2<T extends { new (s: number): string}>(s: T) {
    let p = new s(32);
AssertType(p, "string");
AssertType(new s(32), "string");
AssertType(s, "T");
AssertType(32, "int");

    let p: string;
AssertType(p, "string");
}

// Construct expression of void returning function
function fnVoid(): void { }
let t = new fnVoid();
AssertType(t, "any");
AssertType(new fnVoid(), "any");
AssertType(fnVoid, "() => void");

let t: any;
AssertType(t, "any");

// Chained new expressions
let nested = new (new (new nestedCtor())())();
AssertType(nested, "nestedCtor");
AssertType(new (new (new nestedCtor())())(), "nestedCtor");
AssertType((new (new nestedCtor())()), "nestedCtor");
AssertType(new (new nestedCtor())(), "nestedCtor");
AssertType((new nestedCtor()), "nestedCtor");
AssertType(new nestedCtor(), "nestedCtor");
AssertType(nestedCtor, "nestedCtor");

let n = new nested();
AssertType(n, "nestedCtor");
AssertType(new nested(), "nestedCtor");
AssertType(nested, "nestedCtor");

let n = new nested();
AssertType(n, "nestedCtor");
AssertType(new nested(), "nestedCtor");
AssertType(nested, "nestedCtor");


