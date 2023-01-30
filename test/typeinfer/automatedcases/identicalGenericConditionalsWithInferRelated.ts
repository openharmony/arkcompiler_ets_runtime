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

// === tests/cases/compiler/identicalGenericConditionalsWithInferRelated.ts ===
declare function AssertType(value:any, type:string):void;
function f<X>(arg: X) {
    type Cond1 = X extends [infer A] ? A : never;
AssertType(Cond1, "X extends [infer A] ? A : never");

    type Cond2 = X extends [infer A] ? A : never;
AssertType(Cond2, "X extends [infer A] ? A : never");

    let x: Cond1 = null as any;
AssertType(x, "X extends [infer A] ? A : never");
AssertType(null as any, "any");
AssertType(null, "null");

    let y: Cond2 = null as any;
AssertType(y, "X extends [infer A] ? A : never");
AssertType(null as any, "any");
AssertType(null, "null");

    x = y; // is err, should be ok
AssertType(x = y, "X extends [infer A] ? A : never");
AssertType(x, "X extends [infer A] ? A : never");
AssertType(y, "X extends [infer A] ? A : never");

    y = x; // is err, should be ok
AssertType(y = x, "X extends [infer A] ? A : never");
AssertType(y, "X extends [infer A] ? A : never");
AssertType(x, "X extends [infer A] ? A : never");
}

// repro from https://github.com/microsoft/TypeScript/issues/26627
export type Constructor<T> = new (...args: any[]) => T
export type MappedResult<T> =
    T extends Boolean ? boolean :
    T extends Number ? number :
    T extends String ? string :
    T


interface X {
    decode<C extends Constructor<any>>(ctor: C): MappedResult<C extends Constructor<infer T> ? T : never>
}

class Y implements X {
    decode<C extends Constructor<any>>(ctor: C): MappedResult<C extends Constructor<infer T> ? T : never> {
        throw new Error()
AssertType(new Error(), "Error");
AssertType(Error, "ErrorConstructor");
    }
}


