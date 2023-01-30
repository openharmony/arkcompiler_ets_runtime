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

// === tests/cases/compiler/letDeclarations-access.ts ===
declare function AssertType(value:any, type:string):void;
let x = 0
AssertType(x, "number");
AssertType(0, "int");

// No errors

x = 1;
AssertType(x = 1, "int");
AssertType(x, "number");
AssertType(1, "int");

x += 2;
AssertType(x += 2, "number");
AssertType(x, "number");
AssertType(2, "int");

x -= 3;
AssertType(x -= 3, "number");
AssertType(x, "number");
AssertType(3, "int");

x *= 4;
AssertType(x *= 4, "number");
AssertType(x, "number");
AssertType(4, "int");

x /= 5;
AssertType(x /= 5, "number");
AssertType(x, "number");
AssertType(5, "int");

x %= 6;
AssertType(x %= 6, "number");
AssertType(x, "number");
AssertType(6, "int");

x <<= 7;
AssertType(x <<= 7, "number");
AssertType(x, "number");
AssertType(7, "int");

x >>= 8;
AssertType(x >>= 8, "number");
AssertType(x, "number");
AssertType(8, "int");

x >>>= 9;
AssertType(x >>>= 9, "number");
AssertType(x, "number");
AssertType(9, "int");

x &= 10;
AssertType(x &= 10, "number");
AssertType(x, "number");
AssertType(10, "int");

x |= 11;
AssertType(x |= 11, "number");
AssertType(x, "number");
AssertType(11, "int");

x ^= 12;
AssertType(x ^= 12, "number");
AssertType(x, "number");
AssertType(12, "int");

x++;
AssertType(x++, "number");
AssertType(x, "number");

x--;
AssertType(x--, "number");
AssertType(x, "number");

++x;
AssertType(++x, "number");
AssertType(x, "number");

--x;
AssertType(--x, "number");
AssertType(x, "number");

let a = x + 1;
AssertType(a, "number");
AssertType(x + 1, "number");
AssertType(x, "number");
AssertType(1, "int");

function f(v: number) { }
f(x);
AssertType(f(x), "void");
AssertType(f, "(number) => void");
AssertType(x, "number");

if (x) { }

x;
AssertType(x, "number");

(x);
AssertType((x), "number");
AssertType(x, "number");

-x;
AssertType(-x, "number");
AssertType(x, "number");

+x;
AssertType(+x, "number");
AssertType(x, "number");

x.toString();
AssertType(x.toString(), "string");
AssertType(x.toString, "(?number) => string");


