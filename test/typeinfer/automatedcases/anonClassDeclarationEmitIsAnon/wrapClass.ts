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

// === tests/cases/compiler/wrapClass.ts ===
declare function AssertType(value:any, type:string):void;
export function wrapClass(param: any) {
AssertType(class Wrapped {        foo() {            return param;        }    }, "typeof Wrapped");
AssertType(Wrapped, "typeof Wrapped");
    return class Wrapped {

        foo() {
AssertType(foo, "() => any");

AssertType(param, "any");
            return param;
        }
    }
}

export type Constructor<T = {}> = new (...args: any[]) => T;

export function Timestamped<TBase extends Constructor>(Base: TBase) {
AssertType(class extends Base {        timestamp = Date.now();    }, "{ new (...any[]): (Anonymous class); prototype: Timestamped<any>.(Anonymous class); } & TBase");
AssertType(Base, "{}");
    return class extends Base {

        timestamp = Date.now();
AssertType(timestamp, "number");
AssertType(Date.now(), "number");
AssertType(Date.now, "() => number");

    };
}


