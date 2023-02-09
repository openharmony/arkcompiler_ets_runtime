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

// === tests/cases/conformance/classes/mixinAbstractClasses.ts ===
declare function AssertType(value:any, type:string):void;
interface Mixin {
    mixinMethod(): void;
}

function Mixin<TBaseClass extends abstract new (...args: any) => any>(baseClass: TBaseClass): TBaseClass & (abstract new (...args: any) => Mixin) {
    abstract class MixinClass extends baseClass implements Mixin {
        mixinMethod() {
        }
    }
AssertType(MixinClass, "((abstract new (...any) => MixinClass) & { prototype: Mixin<any>.MixinClass; }) & TBaseClass");
    return MixinClass;
}

class ConcreteBase {
    baseMethod() {}
}

abstract class AbstractBase {
    abstract abstractBaseMethod(): void;
}

class DerivedFromConcrete extends Mixin(ConcreteBase) {
}

const wasConcrete = new DerivedFromConcrete();
AssertType(wasConcrete, "DerivedFromConcrete");
AssertType(new DerivedFromConcrete(), "DerivedFromConcrete");
AssertType(DerivedFromConcrete, "typeof DerivedFromConcrete");

wasConcrete.baseMethod();
AssertType(wasConcrete.baseMethod(), "void");
AssertType(wasConcrete.baseMethod, "() => void");

wasConcrete.mixinMethod();
AssertType(wasConcrete.mixinMethod(), "void");
AssertType(wasConcrete.mixinMethod, "() => void");

class DerivedFromAbstract extends Mixin(AbstractBase) {
    abstractBaseMethod() {}
}

const wasAbstract = new DerivedFromAbstract();
AssertType(wasAbstract, "DerivedFromAbstract");
AssertType(new DerivedFromAbstract(), "DerivedFromAbstract");
AssertType(DerivedFromAbstract, "typeof DerivedFromAbstract");

wasAbstract.abstractBaseMethod();
AssertType(wasAbstract.abstractBaseMethod(), "void");
AssertType(wasAbstract.abstractBaseMethod, "() => void");

wasAbstract.mixinMethod();
AssertType(wasAbstract.mixinMethod(), "void");
AssertType(wasAbstract.mixinMethod, "() => void");


