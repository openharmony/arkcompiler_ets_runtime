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

// === tests/cases/compiler/declarationEmitClassMixinLocalClassDeclaration.ts ===
declare function AssertType(value:any, type:string):void;
export type AnyFunction<Result = any> = (...input: any[]) => Result

export type AnyConstructor<Instance extends object = object, Static extends object = object> =
    (new (...input: any[]) => Instance) & Static


type MixinHelperFunc = <A extends AnyConstructor, T>(required: [A], arg: T) => T extends AnyFunction<infer M> ? M : never


export const Mixin: MixinHelperFunc = null as any
AssertType(Mixin, "MixinHelperFunc");
AssertType(null as any, "any");
AssertType(null, "null");


export class Base {}


export class XmlElement2 extends Mixin(
    [Base],
    (base: AnyConstructor<Base, typeof Base>) => {
        class XmlElement2 extends base {
            num: number = 0
        }
AssertType(XmlElement2, "typeof XmlElement2");
        return XmlElement2;

    }) { }


