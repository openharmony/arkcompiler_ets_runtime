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

// === tests/cases/compiler/capturedLetConstInLoop4_ES6.ts ===
declare function AssertType(value:any, type:string):void;
//======let
export function exportedFoo() {
AssertType(v0 + v00 + v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8, "string");
AssertType(v0 + v00 + v1 + v2 + v3 + v4 + v5 + v6 + v7, "string");
AssertType(v0 + v00 + v1 + v2 + v3 + v4 + v5 + v6, "string");
AssertType(v0 + v00 + v1 + v2 + v3 + v4 + v5, "string");
AssertType(v0 + v00 + v1 + v2 + v3 + v4, "string");
AssertType(v0 + v00 + v1 + v2 + v3, "string");
AssertType(v0 + v00 + v1 + v2, "string");
AssertType(v0 + v00 + v1, "string");
AssertType(v0 + v00, "string");
AssertType(v0, "any");
AssertType(v00, "string");
AssertType(v1, "number");
AssertType(v2, "any");
AssertType(v3, "any");
AssertType(v4, "number");
AssertType(v5, "number");
AssertType(v6, "any");
AssertType(v7, "any");
AssertType(v8, "number");
    return v0 + v00 + v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8;
}

for (let x of []) {
    let v0 = x;
AssertType(v0, "any");
AssertType(x, "any");

    (function() { 
AssertType((function() { return x + v0}), "() => any");
AssertType(function() { return x + v0}, "() => any");
AssertType(x + v0, "any");
AssertType(x, "any");
AssertType(v0, "any");
return x + v0});

    (() => x);    
AssertType((() => x), "() => any");
AssertType(() => x, "() => any");
AssertType(x, "any");
}

for (let x in []) {
    let v00 = x;
AssertType(v00, "string");
AssertType(x, "string");

    (function() { 
AssertType((function() { return x + v00}), "() => string");
AssertType(function() { return x + v00}, "() => string");
AssertType(x + v00, "string");
AssertType(x, "string");
AssertType(v00, "string");
return x + v00});

    (() => x);    
AssertType((() => x), "() => string");
AssertType(() => x, "() => string");
AssertType(x, "string");
}

for (let x = 0; x < 1; ++x) {
    let v1 = x;
AssertType(v1, "number");
AssertType(x, "number");

    (function() { 
AssertType((function() { return x + v1}), "() => number");
AssertType(function() { return x + v1}, "() => number");
AssertType(x + v1, "number");
AssertType(x, "number");
AssertType(v1, "number");
return x + v1});

    (() => x);
AssertType((() => x), "() => number");
AssertType(() => x, "() => number");
AssertType(x, "number");
}

while (1 === 1) {
    let x;
AssertType(x, "any");

    let v2 = x;
AssertType(v2, "any");
AssertType(x, "any");

    (function() { 
AssertType((function() { return x + v2}), "() => any");
AssertType(function() { return x + v2}, "() => any");
AssertType(x + v2, "any");
AssertType(x, "any");
AssertType(v2, "any");
return x + v2});

    (() => x);
AssertType((() => x), "() => any");
AssertType(() => x, "() => any");
AssertType(x, "any");
}

do {
    let x;
AssertType(x, "any");

    let v3 = x;
AssertType(v3, "any");
AssertType(x, "any");

    (function() { 
AssertType((function() { return x + v3}), "() => any");
AssertType(function() { return x + v3}, "() => any");
AssertType(x + v3, "any");
AssertType(x, "any");
AssertType(v3, "any");
return x + v3});

    (() => x);
AssertType((() => x), "() => any");
AssertType(() => x, "() => any");
AssertType(x, "any");

} while (1 === 1)

for (let y = 0; y < 1; ++y) {
    let x = 1;
AssertType(x, "number");
AssertType(1, "int");

    let v4 = x;
AssertType(v4, "number");
AssertType(x, "number");

    (function() { 
AssertType((function() { return x + v4}), "() => number");
AssertType(function() { return x + v4}, "() => number");
AssertType(x + v4, "number");
AssertType(x, "number");
AssertType(v4, "number");
return x + v4});

    (() => x);
AssertType((() => x), "() => number");
AssertType(() => x, "() => number");
AssertType(x, "number");
}

for (let x = 0, y = 1; x < 1; ++x) {
    let v5 = x;
AssertType(v5, "number");
AssertType(x, "number");

    (function() { 
AssertType((function() { return x + y + v5}), "() => number");
AssertType(function() { return x + y + v5}, "() => number");
AssertType(x + y + v5, "number");
AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
AssertType(v5, "number");
return x + y + v5});

    (() => x + y);
AssertType((() => x + y), "() => number");
AssertType(() => x + y, "() => number");
AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
}

while (1 === 1) {
    let x, y;
AssertType(x, "any");
AssertType(y, "any");

    let v6 = x;
AssertType(v6, "any");
AssertType(x, "any");

    (function() { 
AssertType((function() { return x + y + v6}), "() => any");
AssertType(function() { return x + y + v6}, "() => any");
AssertType(x + y + v6, "any");
AssertType(x + y, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(v6, "any");
return x + y + v6});

    (() => x + y);
AssertType((() => x + y), "() => any");
AssertType(() => x + y, "() => any");
AssertType(x + y, "any");
AssertType(x, "any");
AssertType(y, "any");
}

do {
    let x, y;
AssertType(x, "any");
AssertType(y, "any");

    let v7 = x;
AssertType(v7, "any");
AssertType(x, "any");

    (function() { 
AssertType((function() { return x + y + v7}), "() => any");
AssertType(function() { return x + y + v7}, "() => any");
AssertType(x + y + v7, "any");
AssertType(x + y, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(v7, "any");
return x + y + v7});

    (() => x + y);
AssertType((() => x + y), "() => any");
AssertType(() => x + y, "() => any");
AssertType(x + y, "any");
AssertType(x, "any");
AssertType(y, "any");

} while (1 === 1)

for (let y = 0; y < 1; ++y) {
    let x = 1;
AssertType(x, "number");
AssertType(1, "int");

    let v8 = x;
AssertType(v8, "number");
AssertType(x, "number");

    (function() { 
AssertType((function() { return x + y + v8}), "() => number");
AssertType(function() { return x + y + v8}, "() => number");
AssertType(x + y + v8, "number");
AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
AssertType(v8, "number");
return x + y + v8});

    (() => x + y);
AssertType((() => x + y), "() => number");
AssertType(() => x + y, "() => number");
AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
}

//======const
export function exportedFoo2() {
AssertType(v0_c + v00_c + v1_c + v2_c + v3_c + v4_c + v5_c + v6_c + v7_c + v8_c, "string");
AssertType(v0_c + v00_c + v1_c + v2_c + v3_c + v4_c + v5_c + v6_c + v7_c, "string");
AssertType(v0_c + v00_c + v1_c + v2_c + v3_c + v4_c + v5_c + v6_c, "string");
AssertType(v0_c + v00_c + v1_c + v2_c + v3_c + v4_c + v5_c, "string");
AssertType(v0_c + v00_c + v1_c + v2_c + v3_c + v4_c, "string");
AssertType(v0_c + v00_c + v1_c + v2_c + v3_c, "string");
AssertType(v0_c + v00_c + v1_c + v2_c, "string");
AssertType(v0_c + v00_c + v1_c, "string");
AssertType(v0_c + v00_c, "string");
AssertType(v0_c, "any");
AssertType(v00_c, "string");
AssertType(v1_c, "number");
AssertType(v2_c, "number");
AssertType(v3_c, "number");
AssertType(v4_c, "number");
AssertType(v5_c, "number");
AssertType(v6_c, "number");
AssertType(v7_c, "number");
AssertType(v8_c, "number");
    return v0_c + v00_c + v1_c + v2_c + v3_c + v4_c + v5_c + v6_c + v7_c + v8_c;
}

for (const x of []) {
    let v0_c = x;
AssertType(v0_c, "any");
AssertType(x, "any");

    (function() { 
AssertType((function() { return x + v0_c}), "() => any");
AssertType(function() { return x + v0_c}, "() => any");
AssertType(x + v0_c, "any");
AssertType(x, "any");
AssertType(v0_c, "any");
return x + v0_c});

    (() => x);    
AssertType((() => x), "() => any");
AssertType(() => x, "() => any");
AssertType(x, "any");
}

for (const x in []) {
    let v00_c = x;
AssertType(v00_c, "string");
AssertType(x, "string");

    (function() { 
AssertType((function() { return x + v00}), "() => string");
AssertType(function() { return x + v00}, "() => string");
AssertType(x + v00, "string");
AssertType(x, "string");
AssertType(v00, "string");
return x + v00});

    (() => x);    
AssertType((() => x), "() => string");
AssertType(() => x, "() => string");
AssertType(x, "string");
}

for (const x = 0; x < 1;) {
    let v1_c = x;
AssertType(v1_c, "number");
AssertType(x, "int");

    (function() { 
AssertType((function() { return x + v1_c}), "() => number");
AssertType(function() { return x + v1_c}, "() => number");
AssertType(x + v1_c, "number");
AssertType(x, "int");
AssertType(v1_c, "number");
return x + v1_c});

    (() => x);
AssertType((() => x), "() => number");
AssertType(() => x, "() => number");
AssertType(x, "int");
}

while (1 === 1) {
    const x =1;
AssertType(x, "int");
AssertType(1, "int");

    let v2_c = x;
AssertType(v2_c, "number");
AssertType(x, "int");

    (function() { 
AssertType((function() { return x + v2_c}), "() => number");
AssertType(function() { return x + v2_c}, "() => number");
AssertType(x + v2_c, "number");
AssertType(x, "int");
AssertType(v2_c, "number");
return x + v2_c});

    (() => x);
AssertType((() => x), "() => number");
AssertType(() => x, "() => number");
AssertType(x, "int");
}

do {
    const x = 1;
AssertType(x, "int");
AssertType(1, "int");

    let v3_c = x;
AssertType(v3_c, "number");
AssertType(x, "int");

    (function() { 
AssertType((function() { return x + v3_c}), "() => number");
AssertType(function() { return x + v3_c}, "() => number");
AssertType(x + v3_c, "number");
AssertType(x, "int");
AssertType(v3_c, "number");
return x + v3_c});

    (() => x);
AssertType((() => x), "() => number");
AssertType(() => x, "() => number");
AssertType(x, "int");

} while (1 === 1)

for (const y = 0; y < 1;) {
    const x = 1;
AssertType(x, "int");
AssertType(1, "int");

    let v4_c = x;
AssertType(v4_c, "number");
AssertType(x, "int");

    (function() { 
AssertType((function() { return x + v4_c}), "() => number");
AssertType(function() { return x + v4_c}, "() => number");
AssertType(x + v4_c, "number");
AssertType(x, "int");
AssertType(v4_c, "number");
return x + v4_c});

    (() => x);
AssertType((() => x), "() => number");
AssertType(() => x, "() => number");
AssertType(x, "int");
}

for (const x = 0, y = 1; x < 1;) {
    let v5_c = x;
AssertType(v5_c, "number");
AssertType(x, "int");

    (function() { 
AssertType((function() { return x + y + v5_c}), "() => number");
AssertType(function() { return x + y + v5_c}, "() => number");
AssertType(x + y + v5_c, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(v5_c, "number");
return x + y + v5_c});

    (() => x + y);
AssertType((() => x + y), "() => number");
AssertType(() => x + y, "() => number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
}

while (1 === 1) {
    const x = 1, y = 1;
AssertType(x, "int");
AssertType(1, "int");
AssertType(y, "int");
AssertType(1, "int");

    let v6_c = x;
AssertType(v6_c, "number");
AssertType(x, "int");

    (function() { 
AssertType((function() { return x + y + v6_c}), "() => number");
AssertType(function() { return x + y + v6_c}, "() => number");
AssertType(x + y + v6_c, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(v6_c, "number");
return x + y + v6_c});

    (() => x + y);
AssertType((() => x + y), "() => number");
AssertType(() => x + y, "() => number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
}

do {
    const x = 1, y = 1;
AssertType(x, "int");
AssertType(1, "int");
AssertType(y, "int");
AssertType(1, "int");

    let v7_c = x;
AssertType(v7_c, "number");
AssertType(x, "int");

    (function() { 
AssertType((function() { return x + y + v7_c}), "() => number");
AssertType(function() { return x + y + v7_c}, "() => number");
AssertType(x + y + v7_c, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(v7_c, "number");
return x + y + v7_c});

    (() => x + y);
AssertType((() => x + y), "() => number");
AssertType(() => x + y, "() => number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");

} while (1 === 1)

for (const y = 0; y < 1;) {
    const x = 1;
AssertType(x, "int");
AssertType(1, "int");

    let v8_c = x;
AssertType(v8_c, "number");
AssertType(x, "int");

    (function() { 
AssertType((function() { return x + y + v8_c}), "() => number");
AssertType(function() { return x + y + v8_c}, "() => number");
AssertType(x + y + v8_c, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(v8_c, "number");
return x + y + v8_c});

    (() => x + y);
AssertType((() => x + y), "() => number");
AssertType(() => x + y, "() => number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
}


