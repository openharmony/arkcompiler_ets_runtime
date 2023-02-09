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

// === tests/cases/conformance/statements/returnStatements/returnStatements.ts ===
declare function AssertType(value:any, type:string):void;
// all the following should be valid
function fn1(): number { 
AssertType(1, "int");
return 1; 
}

function fn2(): string { 
AssertType('', "string");
return ''; 
}

function fn3(): void { 
AssertType(undefined, "undefined");
return undefined; 
}

function fn4(): void { 
return; }
function fn5(): boolean { 
AssertType(true, "boolean");
return true; 
}

function fn6(): Date { 
AssertType(new Date(12), "Date");
return new Date(12); 

AssertType(Date, "DateConstructor");

AssertType(12, "int");
}

function fn7(): any { 
AssertType(null, "null");
return null; 
}

function fn8(): any { 
return; } // OK, eq. to 'return undefined'

interface I { id: number }
class C implements I {
    id: number;
    dispose() {}
}
class D extends C {
    name: string;
}
function fn10(): I { 
AssertType({ id: 12 }, "{ id: number; }");
AssertType(id, "number");
AssertType(12, "int");
return { id: 12 }; } 

function fn11(): I { 
AssertType(new C(), "C");
return new C(); 

AssertType(C, "typeof C");
}

function fn12(): C { 
AssertType(new D(), "D");
return new D(); 

AssertType(D, "typeof D");
}

function fn13(): C { 
AssertType(null, "null");
return null; 
}


