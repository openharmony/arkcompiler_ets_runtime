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

// === tests/cases/compiler/capturedLetConstInLoop13.ts ===
declare function AssertType(value:any, type:string):void;
class Main {

    constructor() {
        this.register("a", "b", "c");
AssertType(this.register("a", "b", "c"), "void");
AssertType(this.register, "(...string[]) => void");
AssertType(this, "this");
AssertType("a", "string");
AssertType("b", "string");
AssertType("c", "string");
    }

    private register(...names: string[]): void {
        for (let name of names) {
AssertType(name, "string");
AssertType(names, "string[]");

            this.bar({
AssertType(this.bar({                [name + ".a"]: () => { this.foo(name); },            }), "void");
AssertType(this.bar, "(any) => void");
AssertType(this, "this");
AssertType({                [name + ".a"]: () => { this.foo(name); },            }, "{ [string]: () => void; }");

                [name + ".a"]: () => { this.foo(name); },
AssertType([name + ".a"], "() => void");
AssertType(name + ".a", "string");
AssertType(name, "string");
AssertType(".a", "string");
AssertType(() => { this.foo(name); }, "() => void");
AssertType(this.foo(name), "void");
AssertType(this.foo, "(string) => void");
AssertType(this, "this");
AssertType(name, "string");

            });
        }
    }

    private bar(a: any): void { }

    private foo(name: string): void { }

}

new Main();
AssertType(new Main(), "Main");
AssertType(Main, "typeof Main");


