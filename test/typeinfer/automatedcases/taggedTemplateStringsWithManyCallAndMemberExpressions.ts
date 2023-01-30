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

// === tests/cases/conformance/es6/templates/taggedTemplateStringsWithManyCallAndMemberExpressions.ts ===
declare function AssertType(value:any, type:string):void;
interface I {
    (strs: TemplateStringsArray, ...subs: number[]): I;
    member: {
        new (s: string): {
            new (n: number): {
                new (): boolean;
            }
        }
    };
}
let f: I;
AssertType(f, "I");

let x = new new new f `abc${ 0 }def`.member("hello")(42) === true;
AssertType(x, "boolean");
AssertType(new new new f `abc${ 0 }def`.member("hello")(42) === true, "boolean");
AssertType(new new new f `abc${ 0 }def`.member("hello")(42), "boolean");
AssertType(new new f `abc${ 0 }def`.member("hello")(42), "new () => boolean");
AssertType(new f `abc${ 0 }def`.member("hello"), "new (number) => new () => boolean");
AssertType(f `abc${ 0 }def`.member, "new (string) => new (number) => new () => boolean");
AssertType("hello", "string");
AssertType(42, "int");
AssertType(true, "boolean");



