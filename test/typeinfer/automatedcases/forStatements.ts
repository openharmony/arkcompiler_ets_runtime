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

// === tests/cases/conformance/statements/forStatements/forStatements.ts ===
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

for(let aNumber: number = 9.9;;){} 
for(let aString: string = 'this is a string';;){}
for(let aDate: Date = new Date(12);;){}
for(let anObject: Object = new Object();;){}

for(let anAny: any = null;;){}
for(let aSecondAny: any = undefined;;){}
for(let aVoid: void = undefined;;){}

for(let anInterface: I = new C();;){}
for(let aClass: C = new C();;){}
for(let aGenericClass: D<string> = new D<string>();;){}
for(let anObjectLiteral: I = { id: 12 };;){}
for(let anOtherObjectLiteral: { id: number } = new C();;){}

for(let aFunction: typeof F = F;;){}
for(let anOtherFunction: (x: string) => number = F;;){}
for(let aLambda: typeof F = (x) => 2;;){}

for(let aModule: typeof M = M;;){}
for(let aClassInModule: M.A = new M.A();;){}
for(let aFunctionInModule: typeof M.F2 = (x) => 'this is a string';;){}

