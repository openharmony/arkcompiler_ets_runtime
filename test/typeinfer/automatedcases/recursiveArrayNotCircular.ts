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

// === tests/cases/compiler/recursiveArrayNotCircular.ts ===
declare function AssertType(value:any, type:string):void;
type Action<T, P> = P extends void ? { type : T } : { type: T, payload: P }

enum ActionType {
    Foo,
    Bar,
    Baz,
    Batch
}

type ReducerAction =
  | Action<ActionType.Bar, number>
  | Action<ActionType.Baz, boolean>
  | Action<ActionType.Foo, string>
  | Action<ActionType.Batch, ReducerAction[]>

function assertNever(a: never): never {
    throw new Error("Unreachable!");
AssertType(new Error("Unreachable!"), "Error");
AssertType(Error, "ErrorConstructor");
AssertType("Unreachable!", "string");
}

function reducer(action: ReducerAction): void {
    switch(action.type) {
AssertType(action.type, "ActionType");

        case ActionType.Bar:
AssertType(ActionType.Bar, "ActionType.Bar");

            const x: number = action.payload;
AssertType(x, "number");
AssertType(action.payload, "number");

            break;
        case ActionType.Baz:
AssertType(ActionType.Baz, "ActionType.Baz");

            const y: boolean = action.payload;
AssertType(y, "boolean");
AssertType(action.payload, "boolean");

            break;
        case ActionType.Foo:
AssertType(ActionType.Foo, "ActionType.Foo");

            const z: string = action.payload;
AssertType(z, "string");
AssertType(action.payload, "string");

            break;
        case ActionType.Batch:
AssertType(ActionType.Batch, "ActionType.Batch");

            action.payload.map(reducer);
AssertType(action.payload.map(reducer), "void[]");
AssertType(action.payload.map, "<U>((ReducerAction, number, ReducerAction[]) => U, ?any) => U[]");
AssertType(action.payload, "ReducerAction[]");
AssertType(reducer, "(ReducerAction) => void");

            break;
        default: 
AssertType(assertNever(action), "never");
AssertType(assertNever, "(never) => never");
AssertType(action, "never");
return assertNever(action);
    }
}

