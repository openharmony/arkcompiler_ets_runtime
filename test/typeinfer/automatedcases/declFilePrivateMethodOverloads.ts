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

// === tests/cases/compiler/declFilePrivateMethodOverloads.ts ===
declare function AssertType(value:any, type:string):void;

interface IContext {
    someMethod();
}
class c1 {
    private _forEachBindingContext(bindingContext: IContext, fn: (bindingContext: IContext) => void);
    private _forEachBindingContext(bindingContextArray: Array<IContext>, fn: (bindingContext: IContext) => void);
    private _forEachBindingContext(context, fn: (bindingContext: IContext) => void): void {
        // Function here
    }

    private overloadWithArityDifference(bindingContext: IContext);
    private overloadWithArityDifference(bindingContextArray: Array<IContext>, fn: (bindingContext: IContext) => void);
    private overloadWithArityDifference(context): void {
        // Function here
    }
}
declare class c2 {
    private overload1(context, fn);

    private overload2(context);
    private overload2(context, fn);
}

