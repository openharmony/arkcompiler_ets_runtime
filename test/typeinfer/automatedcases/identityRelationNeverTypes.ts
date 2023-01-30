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

// === tests/cases/compiler/identityRelationNeverTypes.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #47996

type Equals<A, B> = (<T>() => T extends B ? 1 : 0) extends (<T>() => T extends A ? 1 : 0) ? true : false;

declare class State<TContext> {
    _context: TContext;
    _value: string;
    matches<TSV extends string>(stateValue: TSV): this is State<TContext> & { value: TSV };
}

function f1(state: State<{ foo: number }>) {
    if (state.matches('a') && state.matches('a.b')) {
AssertType(state.matches('a') && state.matches('a.b'), "boolean");
AssertType(state.matches('a'), "boolean");
AssertType(state.matches, "<TSV extends string>(TSV) => this is State<{ foo: number; }> & { value: TSV; }");
AssertType('a', "string");
AssertType(state.matches('a.b'), "boolean");
AssertType(state.matches, "<TSV extends string>(TSV) => this is State<{ foo: number; }> & { value: TSV; }");
AssertType('a.b', "string");

        state;  // never
AssertType(state, "never");

        type T1 = Equals<typeof state, never>;  // true
AssertType(T1, "boolean");
AssertType(state, "never");

        type T2 = Equals<never, never>;  // true
AssertType(T2, "boolean");
    }
}


