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

// === tests/cases/conformance/types/members/objectTypeWithCallSignatureAppearsToBeFunctionType.ts ===
declare function AssertType(value:any, type:string):void;
// objects with call signatures should be permitted where function types are expected
// no errors expected below

interface I {
    (): void;
}

let i: I;
AssertType(i, "I");

let r2: void = i();
AssertType(r2, "void");
AssertType(i(), "void");
AssertType(i, "I");

let r2b: (x: any, y?: any) => any = i.apply;
AssertType(r2b, "(any, ?any) => any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(i.apply, "(Function, any, ?any) => any");

let b: {
AssertType(b, "() => void");

    (): void;
}

let r4: void = b();
AssertType(r4, "void");
AssertType(b(), "void");
AssertType(b, "() => void");

let rb4: (x: any, y?: any) => any = b.apply;
AssertType(rb4, "(any, ?any) => any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(b.apply, "(Function, any, ?any) => any");


