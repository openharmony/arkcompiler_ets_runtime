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

// === tests/cases/compiler/blockScopedBindingsReassignedInLoop6.ts ===
declare function AssertType(value:any, type:string):void;
function f1() {
    for (let [x, y] = [1, 2]; x < y; ++x, --y) {
AssertType(x, "number");
AssertType(y, "number");
AssertType([1, 2], "[number, number]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(x < y, "boolean");
AssertType(x, "number");
AssertType(y, "number");
AssertType(++x, --y, "number");
AssertType(++x, "number");
AssertType(x, "number");
AssertType(--y, "number");
AssertType(y, "number");

        let a = () => x++ + y++;
AssertType(a, "() => number");
AssertType(() => x++ + y++, "() => number");
AssertType(x++ + y++, "number");
AssertType(x++, "number");
AssertType(x, "number");
AssertType(y++, "number");
AssertType(y, "number");

        if (x == 1)
AssertType(x == 1, "boolean");
AssertType(x, "number");
AssertType(1, "int");

            break;
        else if (y == 2)
AssertType(y == 2, "boolean");
AssertType(y, "number");
AssertType(2, "int");

            y = 5;
AssertType(y = 5, "int");
AssertType(y, "number");
AssertType(5, "int");

        else
            return;
    }
}

function f2() {
    for (let [{a: x, b: {c: y}}] = [{a: 1, b: {c: 2}}]; x < y; ++x, --y) {
AssertType(a, "any");
AssertType(x, "number");
AssertType(b, "any");
AssertType(c, "any");
AssertType(y, "number");
AssertType([{a: 1, b: {c: 2}}], "[{ a: number; b: { c: number; }; }]");
AssertType({a: 1, b: {c: 2}}, "{ a: number; b: { c: number; }; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType(b, "{ c: number; }");
AssertType({c: 2}, "{ c: number; }");
AssertType(c, "number");
AssertType(2, "int");
AssertType(x < y, "boolean");
AssertType(x, "number");
AssertType(y, "number");
AssertType(++x, --y, "number");
AssertType(++x, "number");
AssertType(x, "number");
AssertType(--y, "number");
AssertType(y, "number");

        let a = () => x++ + y++;
AssertType(a, "() => number");
AssertType(() => x++ + y++, "() => number");
AssertType(x++ + y++, "number");
AssertType(x++, "number");
AssertType(x, "number");
AssertType(y++, "number");
AssertType(y, "number");

        if (x == 1)
AssertType(x == 1, "boolean");
AssertType(x, "number");
AssertType(1, "int");

            break;
        else if (y == 2)
AssertType(y == 2, "boolean");
AssertType(y, "number");
AssertType(2, "int");

            y = 5;
AssertType(y = 5, "int");
AssertType(y, "number");
AssertType(5, "int");

        else
            return;
    }
}







