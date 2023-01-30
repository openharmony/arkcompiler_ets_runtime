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

// === tests/cases/compiler/performanceComparisonOfStructurallyIdenticalInterfacesWithGenericSignatures.ts ===
declare function AssertType(value:any, type:string):void;
export declare type ThenArg<T> = T extends any ? any : T extends PromiseLike<infer U> ? U : T;

export interface InterfaceA<T> {
    filter(callback: (newValue: T, oldValue: T) => boolean): InterfaceA<T>;
    map<D>(callback: (value: T) => D): InterfaceA<D>;
    await<R extends ThenArg<T>>(): InterfaceA<R>;
    awaitLatest<R extends ThenArg<T>>(): InterfaceA<R>;
    awaitOrdered<R extends ThenArg<T>>(): InterfaceA<R>;
    awaitOrdered2<R extends ThenArg<T>>(): InterfaceA<R>;
    awaitOrdered3<R extends ThenArg<T>>(): InterfaceA<R>;
    awaitOrdered4<R extends ThenArg<T>>(): InterfaceA<R>;
    awaitOrdered5<R extends ThenArg<T>>(): InterfaceA<R>;
    awaitOrdered6<R extends ThenArg<T>>(): InterfaceA<R>;
    awaitOrdered7<R extends ThenArg<T>>(): InterfaceA<R>;
    awaitOrdered8<R extends ThenArg<T>>(): InterfaceA<R>;
    awaitOrdered9<R extends ThenArg<T>>(): InterfaceA<R>;
}

export interface InterfaceB<T> extends InterfaceA<T> {
    map<D>(callback: (value: T) => D): InterfaceB<D>;
    await<R extends ThenArg<T>>(): InterfaceB<R>;
    awaitLatest<R extends ThenArg<T>>(): InterfaceB<R>;
    awaitOrdered<R extends ThenArg<T>>(): InterfaceB<R>;
    awaitOrdered2<R extends ThenArg<T>>(): InterfaceB<R>;
    awaitOrdered3<R extends ThenArg<T>>(): InterfaceB<R>;
    awaitOrdered4<R extends ThenArg<T>>(): InterfaceB<R>;
    awaitOrdered5<R extends ThenArg<T>>(): InterfaceB<R>;
    awaitOrdered6<R extends ThenArg<T>>(): InterfaceB<R>;
    awaitOrdered7<R extends ThenArg<T>>(): InterfaceB<R>;
    awaitOrdered8<R extends ThenArg<T>>(): InterfaceB<R>;
    awaitOrdered9<R extends ThenArg<T>>(): InterfaceB<R>;
}

export class A<T> implements InterfaceB<T> {
    public filter(callback: (newValue: T, oldValue: T) => boolean): B<T> {
AssertType(undefined as any, "any");
AssertType(undefined, "undefined");
        return undefined as any;
    }

    public map<D>(callback: (value: T) => D): B<D> {
AssertType(undefined as any, "any");
AssertType(undefined, "undefined");
        return undefined as any;
    }

    public await<R extends ThenArg<T>>(): B<R> {
AssertType(undefined as any, "any");
AssertType(undefined, "undefined");
        return undefined as any;
    }

    public awaitOrdered<R extends ThenArg<T>>(): B<R> {
AssertType(undefined as any, "any");
AssertType(undefined, "undefined");
        return undefined as any;
    }

    public awaitOrdered2<R extends ThenArg<T>>(): B<R> {
AssertType(undefined as any, "any");
AssertType(undefined, "undefined");
        return undefined as any;
    }

    public awaitOrdered3<R extends ThenArg<T>>(): B<R> {
AssertType(undefined as any, "any");
AssertType(undefined, "undefined");
        return undefined as any;
    }

    public awaitOrdered4<R extends ThenArg<T>>(): B<R> {
AssertType(undefined as any, "any");
AssertType(undefined, "undefined");
        return undefined as any;
    }

    public awaitOrdered5<R extends ThenArg<T>>(): B<R> {
AssertType(undefined as any, "any");
AssertType(undefined, "undefined");
        return undefined as any;
    }

    public awaitOrdered6<R extends ThenArg<T>>(): B<R> {
AssertType(undefined as any, "any");
AssertType(undefined, "undefined");
        return undefined as any;
    }

    public awaitOrdered7<R extends ThenArg<T>>(): B<R> {
AssertType(undefined as any, "any");
AssertType(undefined, "undefined");
        return undefined as any;
    }

    public awaitOrdered8<R extends ThenArg<T>>(): B<R> {
AssertType(undefined as any, "any");
AssertType(undefined, "undefined");
        return undefined as any;
    }

    public awaitOrdered9<R extends ThenArg<T>>(): B<R> {
AssertType(undefined as any, "any");
AssertType(undefined, "undefined");
        return undefined as any;
    }

    public awaitLatest<R extends ThenArg<T>>(): B<R> {
AssertType(undefined as any, "any");
AssertType(undefined, "undefined");
        return undefined as any;
    }
}

export class B<T> extends A<T> { }

