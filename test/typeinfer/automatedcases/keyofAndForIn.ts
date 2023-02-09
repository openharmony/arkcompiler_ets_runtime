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

// === tests/cases/conformance/types/keyof/keyofAndForIn.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #12513

function f1<K extends string, T>(obj: { [P in K]: T }, k: K) {
    const b = k in obj;
AssertType(b, "boolean");
AssertType(k in obj, "boolean");
AssertType(k, "K");
AssertType(obj, "{ [P in K]: T; }");

    let k1: K;
AssertType(k1, "K");

    for (k1 in obj) {
AssertType(k1, "K");
AssertType(obj, "{ [P in K]: T; }");

        let x1 = obj[k1];
AssertType(x1, "{ [P in K]: T; }[K]");
AssertType(obj[k1], "{ [P in K]: T; }[K]");
AssertType(obj, "{ [P in K]: T; }");
AssertType(k1, "K");
    }
    for (let k2 in obj) {
AssertType(k2, "Extract<K, string>");
AssertType(obj, "{ [P in K]: T; }");

        let x2 = obj[k2];
AssertType(x2, "{ [P in K]: T; }[Extract<K, string>]");
AssertType(obj[k2], "{ [P in K]: T; }[Extract<K, string>]");
AssertType(obj, "{ [P in K]: T; }");
AssertType(k2, "Extract<K, string>");
    }
}

function f2<T>(obj: { [P in keyof T]: T[P] }, k: keyof T) {
    const b = k in obj;
AssertType(b, "boolean");
AssertType(k in obj, "boolean");
AssertType(k, "keyof T");
AssertType(obj, "{ [P in keyof T]: T[P]; }");

    let k1: keyof T;
AssertType(k1, "keyof T");

    for (k1 in obj) {
AssertType(k1, "keyof T");
AssertType(obj, "{ [P in keyof T]: T[P]; }");

        let x1 = obj[k1];
AssertType(x1, "{ [P in keyof T]: T[P]; }[keyof T]");
AssertType(obj[k1], "{ [P in keyof T]: T[P]; }[keyof T]");
AssertType(obj, "{ [P in keyof T]: T[P]; }");
AssertType(k1, "keyof T");
    }
    for (let k2 in obj) {
AssertType(k2, "Extract<keyof T, string>");
AssertType(obj, "{ [P in keyof T]: T[P]; }");

        let x2 = obj[k2];
AssertType(x2, "{ [P in keyof T]: T[P]; }[Extract<keyof T, string>]");
AssertType(obj[k2], "{ [P in keyof T]: T[P]; }[Extract<keyof T, string>]");
AssertType(obj, "{ [P in keyof T]: T[P]; }");
AssertType(k2, "Extract<keyof T, string>");
    }
}

function f3<T, K extends keyof T>(obj: { [P in K]: T[P] }, k: K) {
    const b = k in obj;
AssertType(b, "boolean");
AssertType(k in obj, "boolean");
AssertType(k, "K");
AssertType(obj, "{ [P in K]: T[P]; }");

    let k1: K;
AssertType(k1, "K");

    for (k1 in obj) {
AssertType(k1, "K");
AssertType(obj, "{ [P in K]: T[P]; }");

        let x1 = obj[k1];
AssertType(x1, "{ [P in K]: T[P]; }[K]");
AssertType(obj[k1], "{ [P in K]: T[P]; }[K]");
AssertType(obj, "{ [P in K]: T[P]; }");
AssertType(k1, "K");
    }
    for (let k2 in obj) {
AssertType(k2, "Extract<K, string>");
AssertType(obj, "{ [P in K]: T[P]; }");

        let x2 = obj[k2];
AssertType(x2, "{ [P in K]: T[P]; }[Extract<K, string>]");
AssertType(obj[k2], "{ [P in K]: T[P]; }[Extract<K, string>]");
AssertType(obj, "{ [P in K]: T[P]; }");
AssertType(k2, "Extract<K, string>");
    }
}

