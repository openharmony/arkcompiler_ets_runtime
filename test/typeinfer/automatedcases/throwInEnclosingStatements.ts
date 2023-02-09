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

// === tests/cases/conformance/statements/throwStatements/throwInEnclosingStatements.ts ===
declare function AssertType(value:any, type:string):void;
function fn(x) {
    throw x;
AssertType(x, "any");
}

<T>(x: T) => { throw x; 
AssertType(<T>(x: T) => { throw x; }, "<T>(T) => never");

AssertType(x, "T");

AssertType(x, "T");
}

let y: string;
AssertType(y, "string");

switch (y) {
    case 'a':
        throw y;
    default:
        throw y;
}

let z = 0;
AssertType(z, "number");
AssertType(0, "int");

while (z < 10) {
    throw z;
AssertType(z, "number");
}

for (let i = 0; ;) { throw i; 
AssertType(i, "number");
}

for (let idx in {}) { throw idx; 
AssertType(idx, "string");
}

do { throw null; }while(true)
AssertType(null, "null");

let j = 0;
AssertType(j, "number");
AssertType(0, "int");

while (j < 0) { throw j; 
AssertType(j, "number");
}

class C<T> {
    private value: T;
    biz() {
        throw this.value;
AssertType(this.value, "T");
AssertType(this, "this");
    }

    constructor() {
        throw this;
AssertType(this, "this");
    }
}

let aa = {
AssertType(aa, "{ id: number; biz(): never; }");
AssertType({    id:12,    biz() {        throw this;    }}, "{ id: number; biz(): never; }");

    id:12,
AssertType(id, "number");
AssertType(12, "int");

    biz() {
AssertType(biz, "() => never");

        throw this;
AssertType(this, "any");
    }
}


