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

// === tests/cases/compiler/sourceMapValidationClass.ts ===
declare function AssertType(value:any, type:string):void;
class Greeter {
    constructor(public greeting: string, ...b: string[]) {
    }
    greet() {
AssertType("<h1>" + this.greeting + "</h1>", "string");
AssertType("<h1>" + this.greeting, "string");
AssertType("<h1>", "string");
AssertType(this.greeting, "string");
AssertType(this, "this");
AssertType("</h1>", "string");
        return "<h1>" + this.greeting + "</h1>";
    }
    private x: string;
    private x1: number = 10;
    private fn() {
AssertType(this.greeting, "string");
AssertType(this, "this");
        return this.greeting;
    }
    get greetings() {
AssertType(this.greeting, "string");
AssertType(this, "this");
        return this.greeting;
    }
    set greetings(greetings: string) {
        this.greeting = greetings;
AssertType(this.greeting = greetings, "string");
AssertType(this.greeting, "string");
AssertType(this, "this");
AssertType(greetings, "string");
    }
}

