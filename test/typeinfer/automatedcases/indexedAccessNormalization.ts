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

// === tests/cases/compiler/indexedAccessNormalization.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from from #43152

type MyMap<M extends object> = {
    [K in keyof M]: {
        x: number
    }
}

declare function g<T>(value?: T): void;

function f1<M extends object>(mymap: MyMap<M>, k: keyof M) {
    const elemofM = mymap[k];
AssertType(elemofM, "MyMap<M>[keyof M]");
AssertType(mymap[k], "MyMap<M>[keyof M]");
AssertType(mymap, "MyMap<M>");
AssertType(k, "keyof M");

    g(elemofM);
AssertType(g(elemofM), "void");
AssertType(g, "<T>(?union) => void");
AssertType(elemofM, "MyMap<M>[keyof M]");
}

function f2<M extends object>(mymap: MyMap<M>, k: keyof M, z: { x: number }) {
    const q1: MyMap<M>[keyof M] = z;
AssertType(q1, "MyMap<M>[keyof M]");
AssertType(z, "{ x: number; }");

    const q2: MyMap<M>[keyof M] | undefined = z;
AssertType(q2, "union");
AssertType(z, "{ x: number; }");

    const q3: MyMap<M>[keyof M] | string = z;
AssertType(q3, "union");
AssertType(z, "{ x: number; }");
}


