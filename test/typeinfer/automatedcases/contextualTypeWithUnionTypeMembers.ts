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

// === tests/cases/conformance/types/union/contextualTypeWithUnionTypeMembers.ts ===
declare function AssertType(value:any, type:string):void;
//When used as a contextual type, a union type U has those members that are present in any of 
// its constituent types, with types that are unions of the respective members in the constituent types. 
interface I1<T> {
    commonMethodType(a: string): string;
    commonPropertyType: string;
    commonMethodWithTypeParameter(a: T): T;

    methodOnlyInI1(a: string): string;
    propertyOnlyInI1: string;
}
interface I2<T> {
    commonMethodType(a: string): string;
    commonPropertyType: string;
    commonMethodWithTypeParameter(a: T): T;

    methodOnlyInI2(a: string): string;
    propertyOnlyInI2: string;
}

// Let S be the set of types in U that has a property P.
// If S is not empty, U has a property P of a union type of the types of P from each type in S.
let i1: I1<number>;
AssertType(i1, "I1<number>");

let i2: I2<number>;
AssertType(i2, "I2<number>");

let i1Ori2: I1<number> | I2<number> = i1;
AssertType(i1Ori2, "union");
AssertType(i1, "I1<number>");

let i1Ori2: I1<number> | I2<number> = i2;
AssertType(i1Ori2, "union");
AssertType(i2, "I2<number>");

let i1Ori2: I1<number> | I2<number> = { // Like i1
AssertType(i1Ori2, "union");
AssertType({ // Like i1    commonPropertyType: "hello",    commonMethodType: a=> a,    commonMethodWithTypeParameter: a => a,    methodOnlyInI1: a => a,    propertyOnlyInI1: "Hello",}, "{ commonPropertyType: string; commonMethodType: (string) => string; commonMethodWithTypeParameter: (number) => number; methodOnlyInI1: (string) => string; propertyOnlyInI1: string; }");

    commonPropertyType: "hello",
AssertType(commonPropertyType, "string");
AssertType("hello", "string");

    commonMethodType: a=> a,
AssertType(commonMethodType, "(string) => string");
AssertType(a=> a, "(string) => string");
AssertType(a, "string");
AssertType(a, "string");

    commonMethodWithTypeParameter: a => a,
AssertType(commonMethodWithTypeParameter, "(number) => number");
AssertType(a => a, "(number) => number");
AssertType(a, "number");
AssertType(a, "number");

    methodOnlyInI1: a => a,
AssertType(methodOnlyInI1, "(string) => string");
AssertType(a => a, "(string) => string");
AssertType(a, "string");
AssertType(a, "string");

    propertyOnlyInI1: "Hello",
AssertType(propertyOnlyInI1, "string");
AssertType("Hello", "string");

};
let i1Ori2: I1<number> | I2<number> = { // Like i2
AssertType(i1Ori2, "union");
AssertType({ // Like i2    commonPropertyType: "hello",    commonMethodType: a=> a,    commonMethodWithTypeParameter: a => a,    methodOnlyInI2: a => a,    propertyOnlyInI2: "Hello",}, "{ commonPropertyType: string; commonMethodType: (string) => string; commonMethodWithTypeParameter: (number) => number; methodOnlyInI2: (string) => string; propertyOnlyInI2: string; }");

    commonPropertyType: "hello",
AssertType(commonPropertyType, "string");
AssertType("hello", "string");

    commonMethodType: a=> a,
AssertType(commonMethodType, "(string) => string");
AssertType(a=> a, "(string) => string");
AssertType(a, "string");
AssertType(a, "string");

    commonMethodWithTypeParameter: a => a,
AssertType(commonMethodWithTypeParameter, "(number) => number");
AssertType(a => a, "(number) => number");
AssertType(a, "number");
AssertType(a, "number");

    methodOnlyInI2: a => a,
AssertType(methodOnlyInI2, "(string) => string");
AssertType(a => a, "(string) => string");
AssertType(a, "string");
AssertType(a, "string");

    propertyOnlyInI2: "Hello",
AssertType(propertyOnlyInI2, "string");
AssertType("Hello", "string");

};
let i1Ori2: I1<number> | I2<number> = { // Like i1 and i2 both
AssertType(i1Ori2, "union");
AssertType({ // Like i1 and i2 both    commonPropertyType: "hello",    commonMethodType: a=> a,    commonMethodWithTypeParameter: a => a,    methodOnlyInI1: a => a,    propertyOnlyInI1: "Hello",    methodOnlyInI2: a => a,    propertyOnlyInI2: "Hello",}, "{ commonPropertyType: string; commonMethodType: (string) => string; commonMethodWithTypeParameter: (number) => number; methodOnlyInI1: (string) => string; propertyOnlyInI1: string; methodOnlyInI2: (string) => string; propertyOnlyInI2: string; }");

    commonPropertyType: "hello",
AssertType(commonPropertyType, "string");
AssertType("hello", "string");

    commonMethodType: a=> a,
AssertType(commonMethodType, "(string) => string");
AssertType(a=> a, "(string) => string");
AssertType(a, "string");
AssertType(a, "string");

    commonMethodWithTypeParameter: a => a,
AssertType(commonMethodWithTypeParameter, "(number) => number");
AssertType(a => a, "(number) => number");
AssertType(a, "number");
AssertType(a, "number");

    methodOnlyInI1: a => a,
AssertType(methodOnlyInI1, "(string) => string");
AssertType(a => a, "(string) => string");
AssertType(a, "string");
AssertType(a, "string");

    propertyOnlyInI1: "Hello",
AssertType(propertyOnlyInI1, "string");
AssertType("Hello", "string");

    methodOnlyInI2: a => a,
AssertType(methodOnlyInI2, "(string) => string");
AssertType(a => a, "(string) => string");
AssertType(a, "string");
AssertType(a, "string");

    propertyOnlyInI2: "Hello",
AssertType(propertyOnlyInI2, "string");
AssertType("Hello", "string");

};

let arrayI1OrI2: Array<I1<number> | I2<number>> = [i1, i2, { // Like i1
AssertType(arrayI1OrI2, "(union)[]");
AssertType([i1, i2, { // Like i1        commonPropertyType: "hello",        commonMethodType: a=> a,        commonMethodWithTypeParameter: a => a,        methodOnlyInI1: a => a,        propertyOnlyInI1: "Hello",    },    { // Like i2        commonPropertyType: "hello",        commonMethodType: a=> a,        commonMethodWithTypeParameter: a => a,        methodOnlyInI2: a => a,        propertyOnlyInI2: "Hello",    }, { // Like i1 and i2 both        commonPropertyType: "hello",        commonMethodType: a=> a,        commonMethodWithTypeParameter: a => a,        methodOnlyInI1: a => a,        propertyOnlyInI1: "Hello",        methodOnlyInI2: a => a,        propertyOnlyInI2: "Hello",    }], "(union)[]");
AssertType(i1, "I1<number>");
AssertType(i2, "I2<number>");
AssertType({ // Like i1        commonPropertyType: "hello",        commonMethodType: a=> a,        commonMethodWithTypeParameter: a => a,        methodOnlyInI1: a => a,        propertyOnlyInI1: "Hello",    }, "{ commonPropertyType: string; commonMethodType: (string) => string; commonMethodWithTypeParameter: (number) => number; methodOnlyInI1: (string) => string; propertyOnlyInI1: string; }");

        commonPropertyType: "hello",
AssertType(commonPropertyType, "string");
AssertType("hello", "string");

        commonMethodType: a=> a,
AssertType(commonMethodType, "(string) => string");
AssertType(a=> a, "(string) => string");
AssertType(a, "string");
AssertType(a, "string");

        commonMethodWithTypeParameter: a => a,
AssertType(commonMethodWithTypeParameter, "(number) => number");
AssertType(a => a, "(number) => number");
AssertType(a, "number");
AssertType(a, "number");

        methodOnlyInI1: a => a,
AssertType(methodOnlyInI1, "(string) => string");
AssertType(a => a, "(string) => string");
AssertType(a, "string");
AssertType(a, "string");

        propertyOnlyInI1: "Hello",
AssertType(propertyOnlyInI1, "string");
AssertType("Hello", "string");

    },
    { // Like i2
AssertType({ // Like i2        commonPropertyType: "hello",        commonMethodType: a=> a,        commonMethodWithTypeParameter: a => a,        methodOnlyInI2: a => a,        propertyOnlyInI2: "Hello",    }, "{ commonPropertyType: string; commonMethodType: (string) => string; commonMethodWithTypeParameter: (number) => number; methodOnlyInI2: (string) => string; propertyOnlyInI2: string; }");

        commonPropertyType: "hello",
AssertType(commonPropertyType, "string");
AssertType("hello", "string");

        commonMethodType: a=> a,
AssertType(commonMethodType, "(string) => string");
AssertType(a=> a, "(string) => string");
AssertType(a, "string");
AssertType(a, "string");

        commonMethodWithTypeParameter: a => a,
AssertType(commonMethodWithTypeParameter, "(number) => number");
AssertType(a => a, "(number) => number");
AssertType(a, "number");
AssertType(a, "number");

        methodOnlyInI2: a => a,
AssertType(methodOnlyInI2, "(string) => string");
AssertType(a => a, "(string) => string");
AssertType(a, "string");
AssertType(a, "string");

        propertyOnlyInI2: "Hello",
AssertType(propertyOnlyInI2, "string");
AssertType("Hello", "string");

    }, { // Like i1 and i2 both
AssertType({ // Like i1 and i2 both        commonPropertyType: "hello",        commonMethodType: a=> a,        commonMethodWithTypeParameter: a => a,        methodOnlyInI1: a => a,        propertyOnlyInI1: "Hello",        methodOnlyInI2: a => a,        propertyOnlyInI2: "Hello",    }, "{ commonPropertyType: string; commonMethodType: (string) => string; commonMethodWithTypeParameter: (number) => number; methodOnlyInI1: (string) => string; propertyOnlyInI1: string; methodOnlyInI2: (string) => string; propertyOnlyInI2: string; }");

        commonPropertyType: "hello",
AssertType(commonPropertyType, "string");
AssertType("hello", "string");

        commonMethodType: a=> a,
AssertType(commonMethodType, "(string) => string");
AssertType(a=> a, "(string) => string");
AssertType(a, "string");
AssertType(a, "string");

        commonMethodWithTypeParameter: a => a,
AssertType(commonMethodWithTypeParameter, "(number) => number");
AssertType(a => a, "(number) => number");
AssertType(a, "number");
AssertType(a, "number");

        methodOnlyInI1: a => a,
AssertType(methodOnlyInI1, "(string) => string");
AssertType(a => a, "(string) => string");
AssertType(a, "string");
AssertType(a, "string");

        propertyOnlyInI1: "Hello",
AssertType(propertyOnlyInI1, "string");
AssertType("Hello", "string");

        methodOnlyInI2: a => a,
AssertType(methodOnlyInI2, "(string) => string");
AssertType(a => a, "(string) => string");
AssertType(a, "string");
AssertType(a, "string");

        propertyOnlyInI2: "Hello",
AssertType(propertyOnlyInI2, "string");
AssertType("Hello", "string");

    }];

interface I11 {
    commonMethodDifferentReturnType(a: string, b: number): string;
    commonPropertyDifferentType: string;
}
interface I21 {
    commonMethodDifferentReturnType(a: string, b: number): number;
    commonPropertyDifferentType: number;
}
let i11: I11;
AssertType(i11, "I11");

let i21: I21;
AssertType(i21, "I21");

let i11Ori21: I11 | I21 = i11;
AssertType(i11Ori21, "union");
AssertType(i11, "I11");

let i11Ori21: I11 | I21 = i21;
AssertType(i11Ori21, "union");
AssertType(i21, "I21");

let i11Ori21: I11 | I21 = { 
AssertType(i11Ori21, "union");
AssertType({     // Like i1    commonMethodDifferentReturnType: (a, b) => {        let z = a.charAt(b);        return z;      },    commonPropertyDifferentType: "hello",  }, "{ commonMethodDifferentReturnType: (string, number) => string; commonPropertyDifferentType: string; }");

    // Like i1
    commonMethodDifferentReturnType: (a, b) => {
AssertType(commonMethodDifferentReturnType, "(string, number) => string");
AssertType(a, "string");
AssertType(b, "number");
AssertType((a, b) => {        let z = a.charAt(b);        return z;      }, "(string, number) => string");

        let z = a.charAt(b);
AssertType(z, "string");
AssertType(a.charAt(b), "string");
AssertType(a.charAt, "(number) => string");
AssertType(b, "number");

AssertType(z, "string");
        return z;  

    },
    commonPropertyDifferentType: "hello",  
AssertType(commonPropertyDifferentType, "string");
AssertType("hello", "string");

};
let i11Ori21: I11 | I21 = { 
AssertType(i11Ori21, "union");
AssertType({     // Like i2    commonMethodDifferentReturnType: (a, b) => {        let z = a.charCodeAt(b);        return z;    },    commonPropertyDifferentType: 10,}, "{ commonMethodDifferentReturnType: (string, number) => number; commonPropertyDifferentType: number; }");

    // Like i2
    commonMethodDifferentReturnType: (a, b) => {
AssertType(commonMethodDifferentReturnType, "(string, number) => number");
AssertType(a, "string");
AssertType(b, "number");
AssertType((a, b) => {        let z = a.charCodeAt(b);        return z;    }, "(string, number) => number");

        let z = a.charCodeAt(b);
AssertType(z, "number");
AssertType(a.charCodeAt(b), "number");
AssertType(a.charCodeAt, "(number) => number");
AssertType(b, "number");

AssertType(z, "number");
        return z;

    },
    commonPropertyDifferentType: 10,
AssertType(commonPropertyDifferentType, "number");
AssertType(10, "int");

};
let arrayOrI11OrI21: Array<I11 | I21> = [i11, i21, i11 || i21, { 
AssertType(arrayOrI11OrI21, "(union)[]");
AssertType(i11, "I11");
AssertType(i21, "I21");
AssertType(i11 || i21, "union");
AssertType(i11, "I11");
AssertType(i21, "I21");
AssertType({         // Like i1        commonMethodDifferentReturnType: (a, b) => {            let z = a.charAt(b);            return z;        },        commonPropertyDifferentType: "hello",    }, "{ commonMethodDifferentReturnType: (string, number) => string; commonPropertyDifferentType: string; }");
AssertType([i11, i21, i11 || i21, {         // Like i1        commonMethodDifferentReturnType: (a, b) => {            let z = a.charAt(b);            return z;        },        commonPropertyDifferentType: "hello",    }, {         // Like i2        commonMethodDifferentReturnType: (a, b) => {            let z = a.charCodeAt(b);            return z;        },        commonPropertyDifferentType: 10,    }], "(union)[]");

        // Like i1
        commonMethodDifferentReturnType: (a, b) => {
AssertType(commonMethodDifferentReturnType, "(string, number) => string");
AssertType(a, "string");
AssertType(b, "number");
AssertType((a, b) => {            let z = a.charAt(b);            return z;        }, "(string, number) => string");

            let z = a.charAt(b);
AssertType(z, "string");
AssertType(a.charAt(b), "string");
AssertType(a.charAt, "(number) => string");
AssertType(b, "number");

AssertType(z, "string");
            return z;

        },
        commonPropertyDifferentType: "hello",
AssertType(commonPropertyDifferentType, "string");
AssertType("hello", "string");

    }, { 
AssertType({         // Like i2        commonMethodDifferentReturnType: (a, b) => {            let z = a.charCodeAt(b);            return z;        },        commonPropertyDifferentType: 10,    }, "{ commonMethodDifferentReturnType: (string, number) => number; commonPropertyDifferentType: number; }");

        // Like i2
        commonMethodDifferentReturnType: (a, b) => {
AssertType(commonMethodDifferentReturnType, "(string, number) => number");
AssertType(a, "string");
AssertType(b, "number");
AssertType((a, b) => {            let z = a.charCodeAt(b);            return z;        }, "(string, number) => number");

            let z = a.charCodeAt(b);
AssertType(z, "number");
AssertType(a.charCodeAt(b), "number");
AssertType(a.charCodeAt, "(number) => number");
AssertType(b, "number");

AssertType(z, "number");
            return z;

        },
        commonPropertyDifferentType: 10,
AssertType(commonPropertyDifferentType, "number");
AssertType(10, "int");

    }];

