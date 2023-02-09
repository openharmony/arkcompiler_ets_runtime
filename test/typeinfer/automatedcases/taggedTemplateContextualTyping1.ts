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

// === tests/cases/conformance/expressions/contextualTyping/taggedTemplateContextualTyping1.ts ===
declare function AssertType(value:any, type:string):void;
type FuncType = (x: <T>(p: T) => T) => typeof x;

function tempTag1<T>(templateStrs: TemplateStringsArray, f: FuncType, x: T): T;
function tempTag1<T>(templateStrs: TemplateStringsArray, f: FuncType, h: FuncType, x: T): T;
function tempTag1<T>(...rest: any[]): T {
AssertType(undefined, "undefined");
    return undefined;
}

// If contextual typing takes place, these functions should work.
// Otherwise, the arrow functions' parameters will be typed as 'any',
// and it is an error to invoke an any-typed value with type arguments,
// so this test will error.
tempTag1 `${ x => { x<number>(undefined); 
AssertType(tempTag1 `${ x => { x<number>(undefined); return x; }                   }${ 10 }`, "int");
AssertType(tempTag1, "{ <T>(TemplateStringsArray, FuncType, T): T; <T>(TemplateStringsArray, FuncType, FuncType, T): T; }");
AssertType(`${ x => { x<number>(undefined); return x; }                   }${ 10 }`, "string");
AssertType(x => { x<number>(undefined); return x; }, "(<T>(T) => T) => <T>(T) => T");
AssertType(x, "<T>(T) => T");
AssertType(x<number>(undefined), "number");
AssertType(x, "<T>(T) => T");
AssertType(undefined, "undefined");
AssertType(x, "<T>(T) => T");
AssertType(10, "int");
return x; }                   }${ 10 }`;

tempTag1 `${ x => { x<number>(undefined); 
AssertType(tempTag1 `${ x => { x<number>(undefined); return x; }                   }${ y => { y<number>(undefined); return y; }                  }${ 10 }`, "int");
AssertType(tempTag1, "{ <T>(TemplateStringsArray, FuncType, T): T; <T>(TemplateStringsArray, FuncType, FuncType, T): T; }");
AssertType(`${ x => { x<number>(undefined); return x; }                   }${ y => { y<number>(undefined); return y; }                  }${ 10 }`, "string");
AssertType(x => { x<number>(undefined); return x; }, "(<T>(T) => T) => <T>(T) => T");
AssertType(x, "<T>(T) => T");
AssertType(x<number>(undefined), "number");
AssertType(x, "<T>(T) => T");
AssertType(undefined, "undefined");
AssertType(x, "<T>(T) => T");
AssertType(y => { y<number>(undefined); return y; }, "(<T>(T) => T) => <T>(T) => T");
AssertType(y, "<T>(T) => T");
AssertType(y<number>(undefined), "number");
AssertType(y, "<T>(T) => T");
AssertType(undefined, "undefined");
AssertType(y, "<T>(T) => T");
AssertType(10, "int");
return x; }                   }${ y => { y<number>(undefined); return y; }                  }${ 10 }`;

tempTag1 `${ x => { x<number>(undefined); 
AssertType(tempTag1 `${ x => { x<number>(undefined); return x; }                   }${ (y: <T>(p: T) => T) => { y<number>(undefined); return y } }${ undefined }`, "any");
AssertType(tempTag1, "{ <T>(TemplateStringsArray, FuncType, T): T; <T>(TemplateStringsArray, FuncType, FuncType, T): T; }");
AssertType(`${ x => { x<number>(undefined); return x; }                   }${ (y: <T>(p: T) => T) => { y<number>(undefined); return y } }${ undefined }`, "string");
AssertType(x => { x<number>(undefined); return x; }, "(<T>(T) => T) => <T>(T) => T");
AssertType(x, "<T>(T) => T");
AssertType(x<number>(undefined), "number");
AssertType(x, "<T>(T) => T");
AssertType(undefined, "undefined");
AssertType(x, "<T>(T) => T");
AssertType((y: <T>(p: T) => T) => { y<number>(undefined); return y }, "(<T>(T) => T) => <T>(T) => T");
AssertType(y, "<T>(T) => T");
AssertType(p, "T");
AssertType(y<number>(undefined), "number");
AssertType(y, "<T>(T) => T");
AssertType(undefined, "undefined");
AssertType(y, "<T>(T) => T");
AssertType(undefined, "undefined");
return x; }                   }${ (y: <T>(p: T) => T) => { y<number>(undefined); return y } }${ undefined }`;

tempTag1 `${ (x: <T>(p: T) => T) => { x<number>(undefined); 
AssertType(tempTag1 `${ (x: <T>(p: T) => T) => { x<number>(undefined); return x; } }${ y => { y<number>(undefined); return y; }                  }${ undefined }`, "any");
AssertType(tempTag1, "{ <T>(TemplateStringsArray, FuncType, T): T; <T>(TemplateStringsArray, FuncType, FuncType, T): T; }");
AssertType(`${ (x: <T>(p: T) => T) => { x<number>(undefined); return x; } }${ y => { y<number>(undefined); return y; }                  }${ undefined }`, "string");
AssertType((x: <T>(p: T) => T) => { x<number>(undefined); return x; }, "(<T>(T) => T) => <T>(T) => T");
AssertType(x, "<T>(T) => T");
AssertType(p, "T");
AssertType(x<number>(undefined), "number");
AssertType(x, "<T>(T) => T");
AssertType(undefined, "undefined");
AssertType(x, "<T>(T) => T");
AssertType(y => { y<number>(undefined); return y; }, "(<T>(T) => T) => <T>(T) => T");
AssertType(y, "<T>(T) => T");
AssertType(y<number>(undefined), "number");
AssertType(y, "<T>(T) => T");
AssertType(undefined, "undefined");
AssertType(y, "<T>(T) => T");
AssertType(undefined, "undefined");
return x; } }${ y => { y<number>(undefined); return y; }                  }${ undefined }`;


