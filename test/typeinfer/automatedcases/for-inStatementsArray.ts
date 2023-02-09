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

// === tests/cases/conformance/statements/for-inStatements/for-inStatementsArray.ts ===
declare function AssertType(value:any, type:string):void;
let a: Date[];
AssertType(a, "Date[]");

let b: boolean[];
AssertType(b, "boolean[]");

for (let x in a) {
    let a1 = a[x];
AssertType(a1, "Date");
AssertType(a[x], "Date");
AssertType(a, "Date[]");
AssertType(x, "string");

    let a2 = a[(x)];
AssertType(a2, "Date");
AssertType(a[(x)], "Date");
AssertType(a, "Date[]");
AssertType((x), "string");
AssertType(x, "string");

    let a3 = a[+x];
AssertType(a3, "Date");
AssertType(a[+x], "Date");
AssertType(a, "Date[]");
AssertType(+x, "number");
AssertType(x, "string");

    let b1 = b[x];
AssertType(b1, "boolean");
AssertType(b[x], "boolean");
AssertType(b, "boolean[]");
AssertType(x, "string");

    let b2 = b[(x)];
AssertType(b2, "boolean");
AssertType(b[(x)], "boolean");
AssertType(b, "boolean[]");
AssertType((x), "string");
AssertType(x, "string");

    let b3 = b[+x];
AssertType(b3, "boolean");
AssertType(b[+x], "boolean");
AssertType(b, "boolean[]");
AssertType(+x, "number");
AssertType(x, "string");
}

for (let x in a) {
    for (let y in a) {
AssertType(y, "string");
AssertType(a, "Date[]");

        for (let z in a) {
AssertType(z, "string");
AssertType(a, "Date[]");

            let a1 = a[x];
AssertType(a1, "Date");
AssertType(a[x], "Date");
AssertType(a, "Date[]");
AssertType(x, "string");

            let a2 = a[y];
AssertType(a2, "Date");
AssertType(a[y], "Date");
AssertType(a, "Date[]");
AssertType(y, "string");

            let a3 = a[z];
AssertType(a3, "Date");
AssertType(a[z], "Date");
AssertType(a, "Date[]");
AssertType(z, "string");
        }
    }
}

let i: string;
AssertType(i, "string");

let j: string;
AssertType(j, "string");

for (i in a) {
    for (j in b) {
AssertType(j, "string");
AssertType(b, "boolean[]");

        let a1 = a[i];
AssertType(a1, "Date");
AssertType(a[i], "Date");
AssertType(a, "Date[]");
AssertType(i, "string");

        let a2 = a[j];
AssertType(a2, "Date");
AssertType(a[j], "Date");
AssertType(a, "Date[]");
AssertType(j, "string");
    }
}

let s: string;
AssertType(s, "string");

for (let s in a) {
    let a1 = a[s];
AssertType(a1, "Date");
AssertType(a[s], "Date");
AssertType(a, "Date[]");
AssertType(s, "string");
}
for (s in a) {
    let a1 = a[s];
AssertType(a1, "Date");
AssertType(a[s], "Date");
AssertType(a, "Date[]");
AssertType(s, "string");
}


