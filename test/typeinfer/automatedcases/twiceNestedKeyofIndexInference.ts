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

// === tests/cases/compiler/twiceNestedKeyofIndexInference.ts ===
declare function AssertType(value:any, type:string):void;
type Set1<T, K1 extends keyof T> = T extends any[] ? T : Pick<T, Exclude<keyof T, K1>> & {
    [SK1 in K1]-?: Required<Pick<T, SK1>>;
}[K1];

type Set2<T, K1 extends keyof T, K2 extends keyof T[K1]> = T extends any[] ? T : Pick<T, Exclude<keyof T, K1>> & {
    [SK1 in K1]-?: Required<{
        [key in K1]: Set1<T[K1], K2>;
    }>;
}[K1];

declare function set<T, K1 extends keyof T>(source: T, path: [K1], value: T[K1]): Set1<T, K1>;

declare function set<T, K1 extends keyof T, K2 extends keyof T[K1]>(source: T, path: [K1, K2], value: T[K1][K2]): Set2<T, K1, K2>;


interface State {
    a: {
        b: string;
        c: number;
    };
    d: boolean;
}

const state: State = {
AssertType(state, "State");
AssertType({    a: {        b: "",        c: 0,    },    d: false,}, "{ a: { b: string; c: number; }; d: false; }");

    a: {
AssertType(a, "{ b: string; c: number; }");
AssertType({        b: "",        c: 0,    }, "{ b: string; c: number; }");

        b: "",
AssertType(b, "string");
AssertType("", "string");

        c: 0,
AssertType(c, "number");
AssertType(0, "int");

    },
    d: false,
AssertType(d, "boolean");
AssertType(false, "boolean");

};

const newState: State = set(state, ["a", 'b'], 'why'); // shouldn't be an error
AssertType(newState, "State");
AssertType(set(state, ["a", 'b'], 'why'), "Pick<State, "d"> & Required<{ a: Pick<{ b: string; c: number; }, "c"> & Required<Pick<{ b: string; c: number; }, "b">>; }>");
AssertType(set, "{ <T, K1 extends keyof T>(T, [K1], T[K1]): Set1<T, K1>; <T, K1 extends keyof T, K2 extends keyof T[K1]>(T, [K1, K2], T[K1][K2]): Set2<T, K1, K2>; }");
AssertType(state, "State");
AssertType(["a", 'b'], "["a", "b"]");
AssertType("a", "string");
AssertType('b', "string");
AssertType('why', "string");


