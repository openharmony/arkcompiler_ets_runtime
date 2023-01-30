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

// === tests/cases/compiler/functionSubtypingOfVarArgs2.ts ===
declare function AssertType(value:any, type:string):void;
class EventBase {
    private _listeners: { (...args: any[]): void; }[] = [];

    add(listener: (...args: any[]) => void): void {
        this._listeners.push(listener);
AssertType(this._listeners.push(listener), "number");
AssertType(this._listeners.push, "(...((...any[]) => void)[]) => number");
AssertType(this._listeners, "((...any[]) => void)[]");
AssertType(this, "this");
AssertType(listener, "(...any[]) => void");
    }
}

class StringEvent extends EventBase {
    add(listener: (items: string, moreitems: number) => void ) {
        super.add(listener);
AssertType(super.add(listener), "void");
AssertType(super.add, "((...any[]) => void) => void");
AssertType(super, "EventBase");
AssertType(listener, "(string, number) => void");
    }
}


