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

// === tests/cases/conformance/internalModules/moduleBody/moduleWithStatementsOfEveryKind.ts ===
declare function AssertType(value:any, type:string):void;
module A {
    class A { s: string }
    class AA<T> { s: T }
    interface I { id: number }

    class B extends AA<string> implements I { id: number }
    class BB<T> extends A {
        id: number;
    }

    module Module {
        class A { s: string }
    }
    enum Color { Blue, Red }
    let x = 12;
    function F(s: string): number {
AssertType(2, "int");
        return 2;
    }
    let array: I[] = null;
    let fn = (s: string) => {
AssertType('hello ' + s, "string");
AssertType('hello ', "string");
AssertType(s, "string");
        return 'hello ' + s;
    }
    let ol = { s: 'hello', id: 2, isvalid: true };

    declare class DC {
        static x: number;
    }
}

module Y {
    export class A { s: string }
    export class AA<T> { s: T }
    export interface I { id: number }

    export class B extends AA<string> implements I { id: number }
    export class BB<T> extends A {
        id: number;
    }

    export module Module {
        class A { s: string }
    }
    export enum Color { Blue, Red }
    export let x = 12;
    export function F(s: string): number {
AssertType(2, "int");
        return 2;
    }
    export let array: I[] = null;
    export let fn = (s: string) => {
AssertType('hello ' + s, "string");
AssertType('hello ', "string");
AssertType(s, "string");
        return 'hello ' + s;
    }
    export let ol = { s: 'hello', id: 2, isvalid: true };

    export declare class DC {
        static x: number;
    }
}


