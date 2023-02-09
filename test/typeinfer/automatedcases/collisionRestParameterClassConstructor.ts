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

// === tests/cases/compiler/collisionRestParameterClassConstructor.ts ===
declare function AssertType(value:any, type:string):void;
// Constructors
class c1 {
    constructor(_i: number, ...restParameters) { //_i is error
        let _i = 10; // no error
AssertType(_i, "number");
AssertType(10, "int");
    }
}
class c1NoError {
    constructor(_i: number) { // no error
        let _i = 10; // no error
AssertType(_i, "number");
AssertType(10, "int");
    }
}

class c2 {
    constructor(...restParameters) {
        let _i = 10; // no error
AssertType(_i, "number");
AssertType(10, "int");
    }
}
class c2NoError {
    constructor() {
        let _i = 10; // no error
AssertType(_i, "number");
AssertType(10, "int");
    }
}

class c3 {
    constructor(public _i: number, ...restParameters) { //_i is error
        let _i = 10; // no error
AssertType(_i, "number");
AssertType(10, "int");
    }
}
class c3NoError {
    constructor(public _i: number) { // no error
        let _i = 10; // no error
AssertType(_i, "number");
AssertType(10, "int");
    }
}

declare class c4 {
    constructor(_i: number, ...restParameters); // No error - no code gen
}
declare class c4NoError {
    constructor(_i: number);  // no error
}

class c5 {
    constructor(_i: number, ...rest); // no codegen no error
    constructor(_i: string, ...rest); // no codegen no error
    constructor(_i: any, ...rest) { // error
        let _i: any; // no error
AssertType(_i, "any");
    }
}

class c5NoError {
    constructor(_i: number); // no error
    constructor(_i: string); // no error
    constructor(_i: any) { // no error
        let _i: any; // no error
AssertType(_i, "any");
    }
}

declare class c6 {
    constructor(_i: number, ...rest); // no codegen no error
    constructor(_i: string, ...rest); // no codegen no error
}

declare class c6NoError {
    constructor(_i: number); // no error
    constructor(_i: string); // no error
}

