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

// === tests/cases/conformance/statements/throwStatements/throwStatements.ts ===
declare function AssertType(value:any, type:string):void;
// all legal

interface I {
    id: number;
}

class C implements I {
    id: number;
}

class D<T>{
    source: T;
    recurse: D<T>;
    wrapped: D<D<T>>
}

function F(x: string): number { 
AssertType(42, "int");
return 42; 
}

module M {
    export class A {
        name: string;
    }

    export function F2(x: number): string { 
AssertType(x.toString(), "string");
return x.toString(); 

AssertType(x.toString, "(?number) => string");
}
}

let aNumber = 9.9;
AssertType(aNumber, "number");
AssertType(9.9, "double");

throw aNumber;
let aString = 'this is a string';
AssertType(aString, "string");
AssertType('this is a string', "string");

throw aString;
let aDate = new Date(12);
AssertType(aDate, "Date");
AssertType(new Date(12), "Date");
AssertType(Date, "DateConstructor");
AssertType(12, "int");

throw aDate;
let anObject = new Object();
AssertType(anObject, "Object");
AssertType(new Object(), "Object");
AssertType(Object, "ObjectConstructor");

throw anObject;

let anAny = null;
AssertType(anAny, "any");
AssertType(null, "null");

throw anAny;
let anOtherAny = <any> new C();
AssertType(anOtherAny, "any");
AssertType(<any> new C(), "any");
AssertType(new C(), "C");
AssertType(C, "typeof C");

throw anOtherAny;
let anUndefined = undefined;
AssertType(anUndefined, "any");
AssertType(undefined, "undefined");

throw anUndefined;

let aClass = new C();
AssertType(aClass, "C");
AssertType(new C(), "C");
AssertType(C, "typeof C");

throw aClass;
let aGenericClass = new D<string>();
AssertType(aGenericClass, "D<string>");
AssertType(new D<string>(), "D<string>");
AssertType(D, "typeof D");

throw aGenericClass;
let anObjectLiteral = { id: 12 };
AssertType(anObjectLiteral, "{ id: number; }");
AssertType({ id: 12 }, "{ id: number; }");
AssertType(id, "number");
AssertType(12, "int");

throw anObjectLiteral;

let aFunction = F;
AssertType(aFunction, "(string) => number");
AssertType(F, "(string) => number");

throw aFunction;
throw aFunction('');
let aLambda = (x) => 2;
AssertType(aLambda, "(any) => number");
AssertType((x) => 2, "(any) => number");
AssertType(x, "any");
AssertType(2, "int");

throw aLambda;
throw aLambda(1);

let aModule = M;
AssertType(aModule, "typeof M");
AssertType(M, "typeof M");

throw aModule;
throw typeof M;
let aClassInModule = new M.A();
AssertType(aClassInModule, "M.A");
AssertType(new M.A(), "M.A");
AssertType(M.A, "typeof M.A");

throw aClassInModule;
let aFunctionInModule = M.F2;
AssertType(aFunctionInModule, "(number) => string");
AssertType(M.F2, "(number) => string");

throw aFunctionInModule;

// no initializer or annotation, so this is an 'any'
let x;
AssertType(x, "any");

throw x;

// literals
throw 0.0;
throw false;
throw null;
throw undefined;
throw 'a string';
throw function () { 
AssertType('a string', "string");
return 'a string' };

throw <T>(x:T) => 42;
throw { x: 12, y: 13 };
throw [];
throw ['a', ['b']];
throw /[a-z]/;
throw new Date();
throw new C();
throw new Object();
throw new D<number>();


