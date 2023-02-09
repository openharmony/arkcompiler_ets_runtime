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

// === tests/cases/compiler/isDeclarationVisibleNodeKinds.ts ===
declare function AssertType(value:any, type:string):void;
// Function types
module schema {
    export function createValidator1(schema: any): <T>(data: T) => T {
AssertType(undefined, "undefined");
        return undefined;
    }
}

// Constructor types
module schema {
    export function createValidator2(schema: any): new <T>(data: T) => T {
AssertType(undefined, "undefined");
        return undefined;
    }
}

// union types
module schema {
     export function createValidator3(schema: any): number | { new <T>(data: T): T; }  {
AssertType(undefined, "undefined");
        return undefined;
    }
}

// Array types
module schema {
     export function createValidator4(schema: any): { new <T>(data: T): T; }[] {
AssertType(undefined, "undefined");
        return undefined;
    }
}


// TypeLiterals
module schema {
    export function createValidator5(schema: any): { new <T>(data: T): T } {
AssertType(undefined, "undefined");
        return undefined;
    }
}

// Tuple types
module schema {
    export function createValidator6(schema: any): [ new <T>(data: T) => T, number] {
AssertType(undefined, "undefined");
        return undefined;
    }
}

// Paren Types
module schema {
    export function createValidator7(schema: any): (new <T>(data: T)=>T )[] {
AssertType(undefined, "undefined");
        return undefined;
    }
}

// Type reference
module schema {
    export function createValidator8(schema: any): Array<{ <T>(data: T) : T}> {
AssertType(undefined, "undefined");
        return undefined;
    }
}


module schema {
    export class T {
        get createValidator9(): <T>(data: T) => T {
AssertType(undefined, "undefined");
            return undefined;
        }
        
        set createValidator10(v: <T>(data: T) => T) {
        }
    }
}

