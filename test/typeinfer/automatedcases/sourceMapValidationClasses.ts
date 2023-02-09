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

// === tests/cases/compiler/sourceMapValidationClasses.ts ===
declare function AssertType(value:any, type:string):void;
module Foo.Bar {
    "use strict";

    class Greeter {
        constructor(public greeting: string) {
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
    }


    function foo(greeting: string): Greeter {
AssertType(new Greeter(greeting), "Greeter");
AssertType(Greeter, "typeof Greeter");
AssertType(greeting, "string");
        return new Greeter(greeting);
    }

    let greeter = new Greeter("Hello, world!");
    let str = greeter.greet();

    function foo2(greeting: string, ...restGreetings /* more greeting */: string[]) {
        let greeters: Greeter[] = []; /* inline block comment */
AssertType(greeters, "Greeter[]");
AssertType([], "undefined[]");

        greeters[0] = new Greeter(greeting);
AssertType(greeters[0] = new Greeter(greeting), "Greeter");
AssertType(greeters[0], "Greeter");
AssertType(greeters, "Greeter[]");
AssertType(0, "int");
AssertType(new Greeter(greeting), "Greeter");
AssertType(Greeter, "typeof Greeter");
AssertType(greeting, "string");

        for (let i = 0; i < restGreetings.length; i++) {
AssertType(i, "number");
AssertType(0, "int");
AssertType(i < restGreetings.length, "boolean");
AssertType(i, "number");
AssertType(restGreetings.length, "number");
AssertType(i++, "number");
AssertType(i, "number");

            greeters.push(new Greeter(restGreetings[i]));
AssertType(greeters.push(new Greeter(restGreetings[i])), "number");
AssertType(greeters.push, "(...Greeter[]) => number");
AssertType(new Greeter(restGreetings[i]), "Greeter");
AssertType(Greeter, "typeof Greeter");
AssertType(restGreetings[i], "string");
AssertType(restGreetings, "string[]");
AssertType(i, "number");
        }

AssertType(greeters, "Greeter[]");
        return greeters;
    }

    let b = foo2("Hello", "World", "!");
    // This is simple signle line comment
    for (let j = 0; j < b.length; j++) {
        b[j].greet();
AssertType(b[j].greet(), "string");
AssertType(b[j].greet, "() => string");
AssertType(b[j], "Greeter");
AssertType(b, "Greeter[]");
AssertType(j, "number");
    }
}

