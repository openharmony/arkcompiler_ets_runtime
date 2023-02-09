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

// === tests/cases/conformance/async/es6/awaitBinaryExpression/awaitBinaryExpression2_es6.ts ===
declare function AssertType(value:any, type:string):void;
declare let a: boolean;
AssertType(a, "boolean");

declare let p: Promise<boolean>;
AssertType(p, "Promise<boolean>");

declare function before(): void;
declare function after(): void;
async function func(): Promise<void> {
    before();
AssertType(before(), "void");
AssertType(before, "() => void");

    let b = await p && a;
AssertType(b, "boolean");
AssertType(await p && a, "boolean");
AssertType(await p, "boolean");
AssertType(p, "Promise<boolean>");
AssertType(a, "boolean");

    after();
AssertType(after(), "void");
AssertType(after, "() => void");
}

