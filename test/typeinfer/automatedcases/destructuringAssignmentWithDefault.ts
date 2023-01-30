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

// === tests/cases/compiler/destructuringAssignmentWithDefault.ts ===
declare function AssertType(value:any, type:string):void;
const a: { x?: number } = { };
AssertType(a, "{ x?: union; }");
AssertType(x, "union");
AssertType({ }, "{}");

let x = 0;
AssertType(x, "number");
AssertType(0, "int");

({x = 1} = a);
AssertType(({x = 1} = a), "{ x?: union; }");
AssertType({x = 1} = a, "{ x?: union; }");
AssertType({x = 1}, "{ x?: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(a, "{ x?: union; }");

// Repro from #26235

function f1(options?: { color?: string, width?: number }) {
    let { color, width } = options || {};
AssertType(color, "union");
AssertType(width, "union");
AssertType(options || {}, "{ color?: union; width?: union; }");
AssertType(options, "union");
AssertType({}, "{}");

    ({ color, width } = options || {});
AssertType(({ color, width } = options || {}), "{ color?: union; width?: union; }");
AssertType({ color, width } = options || {}, "{ color?: union; width?: union; }");
AssertType({ color, width }, "{ color: union; width: union; }");
AssertType(color, "union");
AssertType(width, "union");
AssertType(options || {}, "{ color?: union; width?: union; }");
AssertType(options, "union");
AssertType({}, "{}");

    let x1 = (options || {}).color;
AssertType(x1, "union");
AssertType((options || {}).color, "union");
AssertType((options || {}), "{ color?: union; width?: union; }");
AssertType(options || {}, "{ color?: union; width?: union; }");
AssertType(options, "union");
AssertType({}, "{}");

    let x2 = (options || {})["color"];
AssertType(x2, "union");
AssertType((options || {})["color"], "union");
AssertType((options || {}), "{ color?: union; width?: union; }");
AssertType(options || {}, "{ color?: union; width?: union; }");
AssertType(options, "union");
AssertType({}, "{}");
AssertType("color", "string");
}

function f2(options?: [string?, number?]) {
    let [str, num] = options || [];
AssertType(str, "union");
AssertType(num, "union");
AssertType(options || [], "[(union)?, (union)?]");
AssertType(options, "union");
AssertType([], "[]");

    [str, num] = options || [];
AssertType([str, num] = options || [], "[(union)?, (union)?]");
AssertType([str, num], "[union, union]");
AssertType(str, "union");
AssertType(num, "union");
AssertType(options || [], "[(union)?, (union)?]");
AssertType(options, "union");
AssertType([], "[]");

    let x1 = (options || {})[0];
AssertType(x1, "union");
AssertType((options || {})[0], "union");
AssertType((options || {}), "union");
AssertType(options || {}, "union");
AssertType(options, "union");
AssertType({}, "{}");
AssertType(0, "int");
}

function f3(options?: { color: string, width: number }) {
    let { color, width } = options || {};
AssertType(color, "union");
AssertType(width, "union");
AssertType(options || {}, "union");
AssertType(options, "union");
AssertType({}, "{}");

    ({ color, width } = options || {});
AssertType(({ color, width } = options || {}), "union");
AssertType({ color, width } = options || {}, "union");
AssertType({ color, width }, "{ color: union; width: union; }");
AssertType(color, "union");
AssertType(width, "union");
AssertType(options || {}, "union");
AssertType(options, "union");
AssertType({}, "{}");

    let x1 = (options || {}).color;
AssertType(x1, "union");
AssertType((options || {}).color, "union");
AssertType((options || {}), "union");
AssertType(options || {}, "union");
AssertType(options, "union");
AssertType({}, "{}");

    let x2 = (options || {})["color"];
AssertType(x2, "union");
AssertType((options || {})["color"], "union");
AssertType((options || {}), "union");
AssertType(options || {}, "union");
AssertType(options, "union");
AssertType({}, "{}");
AssertType("color", "string");
}

function f4(options?: [string, number]) {
    let [str, num] = options || [];
AssertType(str, "union");
AssertType(num, "union");
AssertType(options || [], "union");
AssertType(options, "union");
AssertType([], "[]");

    [str, num] = options || [];
AssertType([str, num] = options || [], "union");
AssertType([str, num], "[union, union]");
AssertType(str, "union");
AssertType(num, "union");
AssertType(options || [], "union");
AssertType(options, "union");
AssertType([], "[]");

    let x1 = (options || {})[0];
AssertType(x1, "union");
AssertType((options || {})[0], "union");
AssertType((options || {}), "union");
AssertType(options || {}, "union");
AssertType(options, "union");
AssertType({}, "{}");
AssertType(0, "int");
}


