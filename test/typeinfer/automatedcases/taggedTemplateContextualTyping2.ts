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

// === tests/cases/conformance/expressions/contextualTyping/taggedTemplateContextualTyping2.ts ===
declare function AssertType(value:any, type:string):void;
type FuncType1 = (x: <T>(p: T) => T) => typeof x;
type FuncType2 = (x: <S, T>(p: T) => T) => typeof x;

function tempTag2(templateStrs: TemplateStringsArray, f: FuncType1, x: number): number;
function tempTag2(templateStrs: TemplateStringsArray, f: FuncType2, h: FuncType2, x: string): string;
function tempTag2(...rest: any[]): any {
AssertType(undefined, "undefined");
    return undefined;
}

// If contextual typing takes place, these functions should work.
// Otherwise, the arrow functions' parameters will be typed as 'any',
// and it is an error to invoke an any-typed value with type arguments,
// so this test will error.
tempTag2 `${ x => { x<number>(undefined); 
AssertType(tempTag2 `${ x => { x<number>(undefined); return x; }         }${ 0 }`, "number");
AssertType(tempTag2, "{ (TemplateStringsArray, FuncType1, number): number; (TemplateStringsArray, FuncType2, FuncType2, string): string; }");
AssertType(`${ x => { x<number>(undefined); return x; }         }${ 0 }`, "string");
AssertType(x => { x<number>(undefined); return x; }, "(<T>(T) => T) => <T>(T) => T");
AssertType(x, "<T>(T) => T");
AssertType(x<number>(undefined), "number");
AssertType(x, "<T>(T) => T");
AssertType(undefined, "undefined");
AssertType(x, "<T>(T) => T");
AssertType(0, "int");
return x; }         }${ 0 }`;

tempTag2 `${ x => { x<number, string>(undefined); 
AssertType(tempTag2 `${ x => { x<number, string>(undefined); return x; } }${ y => { y<string, number>(null); return y; } }${ "hello" }`, "string");
AssertType(tempTag2, "{ (TemplateStringsArray, FuncType1, number): number; (TemplateStringsArray, FuncType2, FuncType2, string): string; }");
AssertType(`${ x => { x<number, string>(undefined); return x; } }${ y => { y<string, number>(null); return y; } }${ "hello" }`, "string");
AssertType(x => { x<number, string>(undefined); return x; }, "(<S, T>(T) => T) => <S, T>(T) => T");
AssertType(x, "<S, T>(T) => T");
AssertType(x<number, string>(undefined), "string");
AssertType(x, "<S, T>(T) => T");
AssertType(undefined, "undefined");
AssertType(x, "<S, T>(T) => T");
AssertType(y => { y<string, number>(null); return y; }, "(<S, T>(T) => T) => <S, T>(T) => T");
AssertType(y, "<S, T>(T) => T");
AssertType(y<string, number>(null), "number");
AssertType(y, "<S, T>(T) => T");
AssertType(null, "null");
AssertType(y, "<S, T>(T) => T");
AssertType("hello", "string");
return x; } }${ y => { y<string, number>(null); return y; } }${ "hello" }`;

tempTag2 `${ x => { x<number, string>(undefined); 
AssertType(tempTag2 `${ x => { x<number, string>(undefined); return x; } }${ undefined }${ "hello" }`, "string");
AssertType(tempTag2, "{ (TemplateStringsArray, FuncType1, number): number; (TemplateStringsArray, FuncType2, FuncType2, string): string; }");
AssertType(`${ x => { x<number, string>(undefined); return x; } }${ undefined }${ "hello" }`, "string");
AssertType(x => { x<number, string>(undefined); return x; }, "(<S, T>(T) => T) => <S, T>(T) => T");
AssertType(x, "<S, T>(T) => T");
AssertType(x<number, string>(undefined), "string");
AssertType(x, "<S, T>(T) => T");
AssertType(undefined, "undefined");
AssertType(x, "<S, T>(T) => T");
AssertType(undefined, "undefined");
AssertType("hello", "string");
return x; } }${ undefined }${ "hello" }`;


