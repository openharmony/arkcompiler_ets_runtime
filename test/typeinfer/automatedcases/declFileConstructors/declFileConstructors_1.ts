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

// === tests/cases/compiler/declFileConstructors_1.ts ===
declare function AssertType(value:any, type:string):void;
class GlobalSimpleConstructor {
    /** This comment should appear for foo*/
    constructor() {
    }
}
class GlobalConstructorWithParameters {
    /** This is comment for function signature*/
    constructor(/** this is comment about a*/a: string,
        /** this is comment for b*/
        b: number) {
        let d = a;
AssertType(d, "string");
AssertType(a, "string");
    }
}

class GlobalConstructorWithRestParamters {
    constructor(a: string, ...rests: string[]) {
AssertType(a + rests.join(""), "string");
AssertType(a, "string");
AssertType(rests.join(""), "string");
AssertType(rests.join, "(?string) => string");
AssertType("", "string");
        return a + rests.join("");
    }
}

class GlobalConstructorWithOverloads {
    constructor(a: string);
    constructor(a: number);
    constructor(a: any) {
    }
}

class GlobalConstructorWithPublicParameterProperty {
    constructor(public x: string) {
    }
}

class GlobalConstructorWithPrivateParameterProperty {
    constructor(private x: string) {
    }
}

class GlobalConstructorWithOptionalParameterProperty {
    constructor(public x?: string) {
    }
}

class GlobalConstructorWithParameterInitializer {
    constructor(public x = "hello") {
    }
}

