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

// === tests/cases/compiler/blockScopedBindingsReassignedInLoop3.ts ===
declare function AssertType(value:any, type:string):void;
for (let x = 1, y = 2; x < y; ++x, --y) {
    let a = () => x++ + y++;
AssertType(a, "() => number");
AssertType(() => x++ + y++, "() => number");
AssertType(x++ + y++, "number");
AssertType(x++, "number");
AssertType(x, "number");
AssertType(y++, "number");
AssertType(y, "number");

    if (x == 1) {
AssertType(x == 1, "boolean");
AssertType(x, "number");
AssertType(1, "int");

        break;
    }
    else {
        for (let a = 1; a < 5; --a) {
AssertType(a, "number");
AssertType(1, "int");
AssertType(a < 5, "boolean");
AssertType(a, "number");
AssertType(5, "int");
AssertType(--a, "number");
AssertType(a, "number");

            let f = () => a;
AssertType(f, "() => number");
AssertType(() => a, "() => number");
AssertType(a, "number");

            if (a) {
AssertType(a, "number");

                a = x;
AssertType(a = x, "number");
AssertType(a, "number");
AssertType(x, "number");

                break;
            }
            else {
                y++;
AssertType(y++, "number");
AssertType(y, "number");
            }
        }
        
        y = 5;
AssertType(y = 5, "int");
AssertType(y, "number");
AssertType(5, "int");
    }
}


for (let x = 1, y = 2; x < y; ++x, --y) {
    let a = () => x++ + y++;
AssertType(a, "() => number");
AssertType(() => x++ + y++, "() => number");
AssertType(x++ + y++, "number");
AssertType(x++, "number");
AssertType(x, "number");
AssertType(y++, "number");
AssertType(y, "number");

    if (x == 1) {
AssertType(x == 1, "boolean");
AssertType(x, "number");
AssertType(1, "int");

        continue;
    }
    else {
        for (let a = 1; a < 5; --a) {
AssertType(a, "number");
AssertType(1, "int");
AssertType(a < 5, "boolean");
AssertType(a, "number");
AssertType(5, "int");
AssertType(--a, "number");
AssertType(a, "number");

            let f = () => a;
AssertType(f, "() => number");
AssertType(() => a, "() => number");
AssertType(a, "number");

            if (a) {
AssertType(a, "number");

                a = x;
AssertType(a = x, "number");
AssertType(a, "number");
AssertType(x, "number");

                continue;
            }
            else {
                y++;
AssertType(y++, "number");
AssertType(y, "number");
            }
        }
        
        y = 5;
AssertType(y = 5, "int");
AssertType(y, "number");
AssertType(5, "int");
    }
}

loop2:
for (let x = 1, y = 2; x < y; ++x, --y) {
    let a = () => x++ + y++;
AssertType(a, "() => number");
AssertType(() => x++ + y++, "() => number");
AssertType(x++ + y++, "number");
AssertType(x++, "number");
AssertType(x, "number");
AssertType(y++, "number");
AssertType(y, "number");

    if (x == 1) {
AssertType(x == 1, "boolean");
AssertType(x, "number");
AssertType(1, "int");

        break loop2;
AssertType(loop2, "any");
    }
    else {
        loop1:
AssertType(loop1, "any");

        for (let a = 1; a < 5; --a) {
AssertType(a, "number");
AssertType(1, "int");
AssertType(a < 5, "boolean");
AssertType(a, "number");
AssertType(5, "int");
AssertType(--a, "number");
AssertType(a, "number");

            let f = () => a;
AssertType(f, "() => number");
AssertType(() => a, "() => number");
AssertType(a, "number");

            if (a) {
AssertType(a, "number");

                a = x;
AssertType(a = x, "number");
AssertType(a, "number");
AssertType(x, "number");

                break loop1;
AssertType(loop1, "any");
            }
            else {
                y++;
AssertType(y++, "number");
AssertType(y, "number");

                break loop2
AssertType(loop2, "any");
            }
        }
        
        y = 5;
AssertType(y = 5, "int");
AssertType(y, "number");
AssertType(5, "int");
    }
}

loop2:
for (let x = 1, y = 2; x < y; ++x, --y) {
    let a = () => x++ + y++;
AssertType(a, "() => number");
AssertType(() => x++ + y++, "() => number");
AssertType(x++ + y++, "number");
AssertType(x++, "number");
AssertType(x, "number");
AssertType(y++, "number");
AssertType(y, "number");

    if (x == 1) {
AssertType(x == 1, "boolean");
AssertType(x, "number");
AssertType(1, "int");

        continue loop2;
AssertType(loop2, "any");
    }
    else {
        loop1:
AssertType(loop1, "any");

        for (let a = 1; a < 5; --a) {
AssertType(a, "number");
AssertType(1, "int");
AssertType(a < 5, "boolean");
AssertType(a, "number");
AssertType(5, "int");
AssertType(--a, "number");
AssertType(a, "number");

            let f = () => a;
AssertType(f, "() => number");
AssertType(() => a, "() => number");
AssertType(a, "number");

            if (a) {
AssertType(a, "number");

                a = x;
AssertType(a = x, "number");
AssertType(a, "number");
AssertType(x, "number");

                continue loop1;
AssertType(loop1, "any");
            }
            else {
                y++;
AssertType(y++, "number");
AssertType(y, "number");

                continue loop2
AssertType(loop2, "any");
            }
        }
        
        y = 5;
AssertType(y = 5, "int");
AssertType(y, "number");
AssertType(5, "int");
    }
}


