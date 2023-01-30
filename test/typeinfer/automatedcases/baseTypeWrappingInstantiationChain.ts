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

// === tests/cases/compiler/baseTypeWrappingInstantiationChain.ts ===
declare function AssertType(value:any, type:string):void;
class CBaseBase<T3> {
    constructor(x: Parameter<T3>) { }
}

class CBase<T2> extends CBaseBase<Wrapper<T2>> {

}

class Parameter<T4> {
    method(t: T4) { }
}

class Wrapper<T5> {
    property: T5;
}

class C<T1> extends CBase<T1> {
    public works() {
        new CBaseBase<Wrapper<T1>>(this);
AssertType(new CBaseBase<Wrapper<T1>>(this), "CBaseBase<Wrapper<T1>>");
AssertType(CBaseBase, "typeof CBaseBase");
AssertType(this, "this");
    }
    public alsoWorks() {
        new CBase<T1>(this); // Should not error, parameter is of type Parameter<Wrapper<T1>>
AssertType(new CBase<T1>(this), "CBase<T1>");
AssertType(CBase, "typeof CBase");
AssertType(this, "this");
    }

    public method(t: Wrapper<T1>) { }
}


