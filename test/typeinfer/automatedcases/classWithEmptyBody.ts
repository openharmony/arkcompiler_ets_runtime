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

// === tests/cases/conformance/classes/classDeclarations/classBody/classWithEmptyBody.ts ===
declare function AssertType(value:any, type:string):void;
class C {
}

let c: C;
AssertType(c, "C");

let o: {} = c;
AssertType(o, "{}");
AssertType(c, "C");

c = 1;
AssertType(c = 1, "int");
AssertType(c, "C");
AssertType(1, "int");

c = { foo: '' 
AssertType(c = { foo: '' }, "{ foo: string; }");

AssertType(c, "C");

AssertType({ foo: '' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('', "string");
}

c = () => { 
AssertType(c = () => { }, "() => void");

AssertType(c, "C");

AssertType(() => { }, "() => void");
}

class D {
    constructor() {
AssertType(1, "int");
        return 1;
    }
}

let d: D;
AssertType(d, "D");

let o: {} = d;
AssertType(o, "{}");
AssertType(d, "D");

d = 1;
AssertType(d = 1, "int");
AssertType(d, "D");
AssertType(1, "int");

d = { foo: '' 
AssertType(d = { foo: '' }, "{ foo: string; }");

AssertType(d, "D");

AssertType({ foo: '' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('', "string");
}

d = () => { 
AssertType(d = () => { }, "() => void");

AssertType(d, "D");

AssertType(() => { }, "() => void");
}


