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

// === tests/cases/compiler/hidingConstructSignatures.ts ===
declare function AssertType(value:any, type:string):void;
interface C {
    (a: string): string;
}

interface D extends C {
    new (a: string): number; // Should be ok
}

interface E {
    new (a: string): {};
}

interface F extends E {
    new (a: string): string;
}

let d: D;
AssertType(d, "D");

d(""); // string
AssertType(d(""), "string");
AssertType(d, "D");
AssertType("", "string");

new d(""); // should be number
AssertType(new d(""), "number");
AssertType(d, "D");
AssertType("", "string");

let f: F;
AssertType(f, "F");

new f(""); // string
AssertType(new f(""), "string");
AssertType(f, "F");
AssertType("", "string");

let e: E;
AssertType(e, "E");

new e(""); // {
AssertType(new e(""), "{}");

AssertType(e, "E");

AssertType("", "string");
}


