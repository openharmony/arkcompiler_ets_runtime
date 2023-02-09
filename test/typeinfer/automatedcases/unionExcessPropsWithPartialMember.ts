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

// === tests/cases/compiler/unionExcessPropsWithPartialMember.ts ===
declare function AssertType(value:any, type:string):void;
interface A {
    unused?: string;
    x: string;
}

interface B {
    x: string;
    y: string;
}

declare let ab: A | B;
AssertType(ab, "union");

declare let a: A;
AssertType(a, "A");

ab = {...a, y: (null as any as string | undefined)}; // Should be allowed, since `y` is missing on `A`
AssertType(ab = {...a, y: (null as any as string | undefined)}, "{ y: union; unused?: union; x: string; }");
AssertType(ab, "union");
AssertType({...a, y: (null as any as string | undefined)}, "{ y: union; unused?: union; x: string; }");
AssertType(a, "A");
AssertType(y, "union");
AssertType((null as any as string | undefined), "union");
AssertType(null as any as string | undefined, "union");
AssertType(null as any, "any");
AssertType(null, "null");


