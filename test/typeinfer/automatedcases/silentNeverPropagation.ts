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

// === tests/cases/compiler/silentNeverPropagation.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #45041

type ModuleWithState<TState> = {
    state: TState;
};

type State = {
    a: number;
};

type MoreState = {
    z: string;
};

declare function createModule<TState, TActions>(state: TState, actions: TActions): ModuleWithState<TState> & TActions;

declare function convert<TState, TActions>(m: ModuleWithState<TState> & TActions): ModuleWithState<TState & MoreState> & TActions;

const breaks = convert(
AssertType(breaks, "ModuleWithState<{ a: number; } & MoreState> & ModuleWithState<{ a: number; }> & { foo(): true; }");
AssertType(convert, "<TState, TActions>(ModuleWithState<TState> & TActions) => ModuleWithState<TState & MoreState> & TActions");
AssertType(convert(    createModule({ a: 12 }, { foo() { return true } })), "ModuleWithState<{ a: number; } & MoreState> & ModuleWithState<{ a: number; }> & { foo(): true; }");

    createModule({ a: 12 }, { foo() { 
AssertType(createModule({ a: 12 }, { foo() { return true } }), "ModuleWithState<{ a: number; }> & { foo(): true; }");
AssertType(createModule, "<TState, TActions>(TState, TActions) => ModuleWithState<TState> & TActions");
AssertType({ a: 12 }, "{ a: number; }");
AssertType(a, "number");
AssertType(12, "int");
AssertType({ foo() { return true } }, "{ foo(): true; }");
AssertType(foo, "() => true");
AssertType(true, "boolean");
return true } })

);

breaks.state.a
AssertType(breaks.state.a, "number");

breaks.state.z
AssertType(breaks.state.z, "string");

breaks.foo()
AssertType(breaks.foo(), "boolean");
AssertType(breaks.foo, "() => true");


