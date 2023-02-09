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

// === tests/cases/conformance/statements/VariableStatements/everyTypeWithAnnotationAndInitializer.ts ===
declare function AssertType(value:any, type:string):void;
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

let aNumber: number = 9.9;
AssertType(aNumber, "number");
AssertType(9.9, "double");

let aString: string = 'this is a string';
AssertType(aString, "string");
AssertType('this is a string', "string");

let aDate: Date = new Date(12);
AssertType(aDate, "Date");
AssertType(new Date(12), "Date");
AssertType(Date, "DateConstructor");
AssertType(12, "int");

let anObject: Object = new Object();
AssertType(anObject, "Object");
AssertType(new Object(), "Object");
AssertType(Object, "ObjectConstructor");

let anAny: any = null;
AssertType(anAny, "any");
AssertType(null, "null");

let aSecondAny: any = undefined;
AssertType(aSecondAny, "any");
AssertType(undefined, "undefined");

let aVoid: void = undefined;
AssertType(aVoid, "void");
AssertType(undefined, "undefined");

let anInterface: I = new C();
AssertType(anInterface, "I");
AssertType(new C(), "C");
AssertType(C, "typeof C");

let aClass: C = new C();
AssertType(aClass, "C");
AssertType(new C(), "C");
AssertType(C, "typeof C");

let aGenericClass: D<string> = new D<string>();
AssertType(aGenericClass, "D<string>");
AssertType(new D<string>(), "D<string>");
AssertType(D, "typeof D");

let anObjectLiteral: I = { id: 12 };
AssertType(anObjectLiteral, "I");
AssertType({ id: 12 }, "{ id: number; }");
AssertType(id, "number");
AssertType(12, "int");

let anOtherObjectLiteral: { id: number } = new C();
AssertType(anOtherObjectLiteral, "{ id: number; }");
AssertType(id, "number");
AssertType(new C(), "C");
AssertType(C, "typeof C");

let aFunction: typeof F = F;
AssertType(aFunction, "(string) => number");
AssertType(F, "(string) => number");
AssertType(F, "(string) => number");

let anOtherFunction: (x: string) => number = F;
AssertType(anOtherFunction, "(string) => number");
AssertType(x, "string");
AssertType(F, "(string) => number");

let aLambda: typeof F = (x) => 2;
AssertType(aLambda, "(string) => number");
AssertType(F, "(string) => number");
AssertType((x) => 2, "(string) => number");
AssertType(x, "string");
AssertType(2, "int");

let aModule: typeof M = M;
AssertType(aModule, "typeof M");
AssertType(M, "typeof M");
AssertType(M, "typeof M");

let aClassInModule: M.A = new M.A();
AssertType(aClassInModule, "M.A");
AssertType(M, "any");
AssertType(new M.A(), "M.A");
AssertType(M.A, "typeof M.A");

let aFunctionInModule: typeof M.F2 = (x) => 'this is a string';
AssertType(aFunctionInModule, "(number) => string");
AssertType(M.F2, "(number) => string");
AssertType(M, "typeof M");
AssertType(F2, "(number) => string");
AssertType((x) => 'this is a string', "(number) => string");
AssertType(x, "number");
AssertType('this is a string', "string");



