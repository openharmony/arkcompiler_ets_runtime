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

// === tests/cases/conformance/declarationEmit/declarationEmitWorkWithInlineComments.ts ===
declare function AssertType(value:any, type:string):void;

export class Foo {
  constructor(
    /** @internal */
    public isInternal1: string,
    /** @internal */ public isInternal2: string, /** @internal */
    public isInternal3: string,
    // @internal
    public isInternal4: string,
    // nothing
    /** @internal */
    public isInternal5: string,
    /* @internal */ public isInternal6: string /* trailing */,
    /* @internal */ public isInternal7: string, /** @internal */
    // not work
    public notInternal1: string,
    // @internal
    /* not work */
    public notInternal2: string,
    /* not work */
    // @internal
    /* not work */
    public notInternal3: string,
  ) { }
}

export class Bar {
  constructor(/* @internal */ public isInternal1: string) {}
}

export class Baz {
  constructor(/* @internal */
    public isInternal: string
  ) {}
}

