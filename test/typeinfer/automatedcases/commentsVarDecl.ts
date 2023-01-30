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

// === tests/cases/compiler/commentsVarDecl.ts ===
declare function AssertType(value:any, type:string):void;
/** Variable comments*/
let myVariable = 10; // This trailing Comment1
AssertType(myVariable, "number");
AssertType(10, "int");

/** This is another letiable comment*/
let anotherVariable = 30;
AssertType(anotherVariable, "number");
AssertType(30, "int");

// shouldn't appear
let aVar = "";
AssertType(aVar, "string");
AssertType("", "string");

/** this is multiline comment
  * All these letiables are of number type */
let anotherAnotherVariable = 70; /* these are multiple trailing comments */ /* multiple trailing comments */
AssertType(anotherAnotherVariable, "number");
AssertType(70, "int");

/** Triple slash multiline comment*/
/** another line in the comment*/
/** comment line 2*/
let x = 70; /* multiline trailing comment 
AssertType(x, "number");
AssertType(70, "int");

this is multiline trailing comment */
/** Triple slash comment on the assignment shouldnt be in .d.ts file*/
x = myVariable;
AssertType(x = myVariable, "number");
AssertType(x, "number");
AssertType(myVariable, "number");

/** triple slash comment1*/
/** jsdocstyle comment - only this comment should be in .d.ts file*/
let n = 30;
AssertType(n, "number");
AssertType(30, "int");

/** let deckaration with comment on type as well*/
let y = /** value comment */ 20;
AssertType(y, "number");
AssertType(20, "int");

/// let deckaration with comment on type as well
let yy =
AssertType(yy, "number");

    /// value comment
    20;
AssertType(20, "int");

/** comment2 */
let z = /** lambda comment */ (x: number, y: number) => x + y;
AssertType(z, "(number, number) => number");
AssertType((x: number, y: number) => x + y, "(number, number) => number");
AssertType(x, "number");
AssertType(y, "number");
AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");

let z2: /** type comment*/ (x: number) => string;
AssertType(z2, "(number) => string");
AssertType(x, "number");

let x2 = z2;
AssertType(x2, "(number) => string");
AssertType(z2, "(number) => string");

let n4: (x: number) => string;
AssertType(n4, "(number) => string");
AssertType(x, "number");

n4 = z2;
AssertType(n4 = z2, "(number) => string");
AssertType(n4, "(number) => string");
AssertType(z2, "(number) => string");


