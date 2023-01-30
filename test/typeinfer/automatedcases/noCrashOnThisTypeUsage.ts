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

// === tests/cases/compiler/noCrashOnThisTypeUsage.ts ===
declare function AssertType(value:any, type:string):void;
interface IListenable {
    changeListeners: Function[] | null
    observe(handler: (change: any, oldValue?: any) => void, fireImmediately?: boolean): void
}

function notifyListeners<T>(listenable: IListenable, change: T) {
}

export class ObservableValue<T> {
    constructor(
        public value: T
    ) {
        const newValue: T = value;
AssertType(newValue, "T");
AssertType(value, "T");

        const oldValue: any = null;
AssertType(oldValue, "any");
AssertType(null, "null");

        notifyListeners(this, {
AssertType(notifyListeners(this, {            type: "update",            object: this,            newValue,            oldValue        }), "void");
AssertType(notifyListeners, "<T>(IListenable, T) => void");
AssertType(this, "this");
AssertType({            type: "update",            object: this,            newValue,            oldValue        }, "{ type: string; object: this; newValue: T; oldValue: any; }");

            type: "update",
AssertType(type, "string");
AssertType("update", "string");

            object: this,
AssertType(object, "this");
AssertType(this, "this");

            newValue,
AssertType(newValue, "T");

            oldValue
AssertType(oldValue, "any");

        });
    }
    changeListeners: Function[] | null = [];
    observe(handler: (change: any, oldValue?: any) => void, fireImmediately?: boolean) {}
}

