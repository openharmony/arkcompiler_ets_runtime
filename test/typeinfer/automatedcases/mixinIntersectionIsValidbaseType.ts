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

// === tests/cases/compiler/mixinIntersectionIsValidbaseType.ts ===
declare function AssertType(value:any, type:string):void;
export type Constructor<T extends object = object> = new (...args: any[]) => T;

export interface Initable {
    init(...args: any[]): void;
}

/**
 * Plain mixin where the superclass must be Initable
 */
export const Serializable = <K extends Constructor<Initable> & Initable>(
AssertType(Serializable, "<K extends Constructor<Initable> & Initable>(K) => { new (...any[]): SerializableLocal; prototype: Serializable<any>.SerializableLocal; init(...any[]): void; } & K");
AssertType(<K extends Constructor<Initable> & Initable>(    SuperClass: K) => {    const LocalMixin = (InnerSuperClass: K) => {        return class SerializableLocal extends InnerSuperClass {        }    };    let ResultClass = LocalMixin(SuperClass);    return ResultClass;}, "<K extends Constructor<Initable> & Initable>(K) => { new (...any[]): SerializableLocal; prototype: Serializable<any>.SerializableLocal; init(...any[]): void; } & K");

    SuperClass: K
AssertType(SuperClass, "K");

) => {
    const LocalMixin = (InnerSuperClass: K) => {
AssertType(LocalMixin, "(K) => { new (...any[]): SerializableLocal; prototype: Serializable<any>.SerializableLocal; init(...any[]): void; } & K");
AssertType(InnerSuperClass, "K");
AssertType((InnerSuperClass: K) => {        return class SerializableLocal extends InnerSuperClass {        }    }, "(K) => { new (...any[]): SerializableLocal; prototype: Serializable<any>.SerializableLocal; init(...any[]): void; } & K");

AssertType(class SerializableLocal extends InnerSuperClass {        }, "{ new (...any[]): SerializableLocal; prototype: Serializable<any>.SerializableLocal; init(...any[]): void; } & K");
AssertType(SerializableLocal, "{ new (...any[]): SerializableLocal; prototype: Serializable<any>.SerializableLocal; init(...any[]): void; } & K");
AssertType(InnerSuperClass, "Initable");
        return class SerializableLocal extends InnerSuperClass {
        }
    };
    let ResultClass = LocalMixin(SuperClass);
AssertType(ResultClass, "{ new (...any[]): SerializableLocal; prototype: Serializable<any>.SerializableLocal; init(...any[]): void; } & K");
AssertType(LocalMixin(SuperClass), "{ new (...any[]): SerializableLocal; prototype: Serializable<any>.SerializableLocal; init(...any[]): void; } & K");
AssertType(LocalMixin, "(K) => { new (...any[]): SerializableLocal; prototype: Serializable<any>.SerializableLocal; init(...any[]): void; } & K");
AssertType(SuperClass, "K");

AssertType(ResultClass, "{ new (...any[]): SerializableLocal; prototype: Serializable<any>.SerializableLocal; init(...any[]): void; } & K");
    return ResultClass;

};

const AMixin = <K extends Constructor<Initable> & Initable>(SuperClass: K) => {
AssertType(AMixin, "<K extends Constructor<Initable> & Initable>(K) => void");
AssertType(<K extends Constructor<Initable> & Initable>(SuperClass: K) => {    let SomeHowOkay = class A extends SuperClass {    };    let SomeHowNotOkay = class A extends Serializable(SuperClass) {    };}, "<K extends Constructor<Initable> & Initable>(K) => void");
AssertType(SuperClass, "K");

    let SomeHowOkay = class A extends SuperClass {
AssertType(SomeHowOkay, "{ new (...any[]): A; prototype: AMixin<any>.A; init(...any[]): void; } & K");
AssertType(class A extends SuperClass {    }, "{ new (...any[]): A; prototype: AMixin<any>.A; init(...any[]): void; } & K");
AssertType(A, "{ new (...any[]): A; prototype: AMixin<any>.A; init(...any[]): void; } & K");
AssertType(SuperClass, "Initable");

    };

    let SomeHowNotOkay = class A extends Serializable(SuperClass) {
AssertType(SomeHowNotOkay, "{ new (...any[]): A; prototype: AMixin<any>.A; init(...any[]): void; } & K");
AssertType(class A extends Serializable(SuperClass) {    }, "{ new (...any[]): A; prototype: AMixin<any>.A; init(...any[]): void; } & K");
AssertType(A, "{ new (...any[]): A; prototype: AMixin<any>.A; init(...any[]): void; } & K");
AssertType(Serializable(SuperClass), "Serializable<K>.SerializableLocal & Initable");
AssertType(Serializable, "<K extends Constructor<Initable> & Initable>(K) => { new (...any[]): SerializableLocal; prototype: Serializable<any>.SerializableLocal; init(...any[]): void; } & K");
AssertType(SuperClass, "K");

    };
};

