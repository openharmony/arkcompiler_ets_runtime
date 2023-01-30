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

// === tests/cases/compiler/contextualTypeLogicalOr.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #18005

let x = 123;
AssertType(x, "number");
AssertType(123, "int");

let a =
AssertType(a, "number[]");

    x && [1, 2, 3, 4] ||
AssertType(x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4], "number[]");
AssertType(x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4], "number[]");
AssertType(x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4], "number[]");
AssertType(x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4], "number[]");
AssertType(x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4], "number[]");
AssertType(x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4], "number[]");
AssertType(x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4], "number[]");
AssertType(x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4], "number[]");
AssertType(x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4], "number[]");
AssertType(x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4], "number[]");
AssertType(x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4], "number[]");
AssertType(x && [1, 2, 3, 4] ||    x && [1, 2, 3, 4], "number[]");
AssertType(x && [1, 2, 3, 4], "number[]");
AssertType(x, "number");
AssertType([1, 2, 3, 4], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    x && [1, 2, 3, 4] ||
AssertType(x && [1, 2, 3, 4], "number[]");
AssertType(x, "number");
AssertType([1, 2, 3, 4], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    x && [1, 2, 3, 4] ||
AssertType(x && [1, 2, 3, 4], "number[]");
AssertType(x, "number");
AssertType([1, 2, 3, 4], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    x && [1, 2, 3, 4] ||
AssertType(x && [1, 2, 3, 4], "number[]");
AssertType(x, "number");
AssertType([1, 2, 3, 4], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    x && [1, 2, 3, 4] ||
AssertType(x && [1, 2, 3, 4], "number[]");
AssertType(x, "number");
AssertType([1, 2, 3, 4], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    x && [1, 2, 3, 4] ||
AssertType(x && [1, 2, 3, 4], "number[]");
AssertType(x, "number");
AssertType([1, 2, 3, 4], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    x && [1, 2, 3, 4] ||
AssertType(x && [1, 2, 3, 4], "number[]");
AssertType(x, "number");
AssertType([1, 2, 3, 4], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    x && [1, 2, 3, 4] ||
AssertType(x && [1, 2, 3, 4], "number[]");
AssertType(x, "number");
AssertType([1, 2, 3, 4], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    x && [1, 2, 3, 4] ||
AssertType(x && [1, 2, 3, 4], "number[]");
AssertType(x, "number");
AssertType([1, 2, 3, 4], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    x && [1, 2, 3, 4] ||
AssertType(x && [1, 2, 3, 4], "number[]");
AssertType(x, "number");
AssertType([1, 2, 3, 4], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    x && [1, 2, 3, 4] ||
AssertType(x && [1, 2, 3, 4], "number[]");
AssertType(x, "number");
AssertType([1, 2, 3, 4], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    x && [1, 2, 3, 4] ||
AssertType(x && [1, 2, 3, 4], "number[]");
AssertType(x, "number");
AssertType([1, 2, 3, 4], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    x && [1, 2, 3, 4];
AssertType(x && [1, 2, 3, 4], "number[]");
AssertType(x, "number");
AssertType([1, 2, 3, 4], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");


