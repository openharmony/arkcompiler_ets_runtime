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

// === tests/cases/compiler/declarationEmitLocalClassDeclarationMixin.ts ===
declare function AssertType(value:any, type:string):void;
interface Constructor<C> { new (...args: any[]): C; }

function mixin<B extends Constructor<{}>>(Base: B) {
    class PrivateMixed extends Base {
        bar = 2;
    }
AssertType(PrivateMixed, "{ new (...any[]): PrivateMixed; prototype: mixin<any>.PrivateMixed; } & B");
    return PrivateMixed;
}

export class Unmixed {
    foo = 1;
}

export const Mixed = mixin(Unmixed);
AssertType(Mixed, "{ new (...any[]): mixin<typeof Unmixed>.PrivateMixed; prototype: mixin<any>.PrivateMixed; } & typeof Unmixed");
AssertType(mixin(Unmixed), "{ new (...any[]): mixin<typeof Unmixed>.PrivateMixed; prototype: mixin<any>.PrivateMixed; } & typeof Unmixed");
AssertType(mixin, "<B extends Constructor<{}>>(B) => { new (...any[]): PrivateMixed; prototype: mixin<any>.PrivateMixed; } & B");
AssertType(Unmixed, "typeof Unmixed");

function Filter<C extends Constructor<{}>>(ctor: C) {
    abstract class FilterMixin extends ctor {
        abstract match(path: string): boolean;
        // other concrete methods, fields, constructor
        thing = 12;
    }
AssertType(FilterMixin, "((abstract new (...any[]) => FilterMixin) & { prototype: Filter<any>.FilterMixin; }) & C");
    return FilterMixin;
}

export class FilteredThing extends Filter(Unmixed) {
    match(path: string) {
AssertType(false, "boolean");
        return false;
    }
}


