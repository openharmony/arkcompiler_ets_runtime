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

// === tests/cases/compiler/genericClassPropertyInheritanceSpecialization.ts ===
declare function AssertType(value:any, type:string):void;
interface KnockoutObservableBase<T> {
    peek(): T;
    (): T;
    (value: T): void;
}

interface KnockoutObservable<T> extends KnockoutObservableBase<T> {
    equalityComparer(a: T, b: T): boolean;
    valueHasMutated(): void;
    valueWillMutate(): void;
}

interface KnockoutObservableArray<T> extends KnockoutObservable<T[]> {
    indexOf(searchElement: T, fromIndex?: number): number;
    slice(start: number, end?: number): T[];
    splice(start: number, deleteCount?: number, ...items: T[]): T[];
    pop(): T;
    push(...items: T[]): void;
    shift(): T;
    unshift(...items: T[]): number;
    reverse(): T[];
    sort(compareFunction?: (a: T, b: T) => number): void;
    replace(oldItem: T, newItem: T): void;
    remove(item: T): T[];
    removeAll(items?: T[]): T[];
    destroy(item: T): void;
    destroyAll(items?: T[]): void;
}

interface KnockoutObservableArrayStatic {
    fn: KnockoutObservableArray<any>;

    <T>(value?: T[]): KnockoutObservableArray<T>;
}

declare module ko {
    export let observableArray: KnockoutObservableArrayStatic;
}

module Portal.Controls.Validators {

    export class Validator<TValue> {
        private _subscription;
        public message: KnockoutObservable<string>;
        public validationState: KnockoutObservable<number>;
        public validate: KnockoutObservable<TValue>;
        constructor(message?: string) { }
        public destroy(): void { }
        public _validate(value: TValue): number {
AssertType(0, "int");
return 0 
}
    }
}

module PortalFx.ViewModels.Controls.Validators {

    export class Validator<TValue> extends Portal.Controls.Validators.Validator<TValue> {

        constructor(message?: string) {
            super(message);
AssertType(super(message), "void");
AssertType(super, "typeof Portal.Controls.Validators.Validator");
AssertType(message, "string");
        }
    }

}

interface Contract<TValue> {

    validators: KnockoutObservableArray<PortalFx.ViewModels.Controls.Validators.Validator<TValue>>;
}


class ViewModel<TValue> implements Contract<TValue> {

    public validators: KnockoutObservableArray<PortalFx.ViewModels.Controls.Validators.Validator<TValue>> = ko.observableArray<PortalFx.ViewModels.Controls.Validators.Validator<TValue>>();
}



