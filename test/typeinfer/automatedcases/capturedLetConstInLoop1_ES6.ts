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

// === tests/cases/compiler/capturedLetConstInLoop1_ES6.ts ===
declare function AssertType(value:any, type:string):void;
//==== let
for (let x in {}) {
    (function() { 
AssertType((function() { return x}), "() => string");
AssertType(function() { return x}, "() => string");
AssertType(x, "string");
return x});

    (() => x);
AssertType((() => x), "() => string");
AssertType(() => x, "() => string");
AssertType(x, "string");
}

for (let x of []) {
    (function() { 
AssertType((function() { return x}), "() => any");
AssertType(function() { return x}, "() => any");
AssertType(x, "any");
return x});

    (() => x);
AssertType((() => x), "() => any");
AssertType(() => x, "() => any");
AssertType(x, "any");
}

for (let x = 0; x < 1; ++x) {
    (function() { 
AssertType((function() { return x}), "() => number");
AssertType(function() { return x}, "() => number");
AssertType(x, "number");
return x});

    (() => x);
AssertType((() => x), "() => number");
AssertType(() => x, "() => number");
AssertType(x, "number");
}

while (1 === 1) {
    let x;
AssertType(x, "any");

    (function() { 
AssertType((function() { return x}), "() => any");
AssertType(function() { return x}, "() => any");
AssertType(x, "any");
return x});

    (() => x);
AssertType((() => x), "() => any");
AssertType(() => x, "() => any");
AssertType(x, "any");
}

do {
    let x;
AssertType(x, "any");

    (function() { 
AssertType((function() { return x}), "() => any");
AssertType(function() { return x}, "() => any");
AssertType(x, "any");
return x});

    (() => x);
AssertType((() => x), "() => any");
AssertType(() => x, "() => any");
AssertType(x, "any");

} while (1 === 1)

for (let y = 0; y < 1; ++y) {
    let x = 1;
AssertType(x, "number");
AssertType(1, "int");

    (function() { 
AssertType((function() { return x}), "() => number");
AssertType(function() { return x}, "() => number");
AssertType(x, "number");
return x});

    (() => x);
AssertType((() => x), "() => number");
AssertType(() => x, "() => number");
AssertType(x, "number");
}

for (let x = 0, y = 1; x < 1; ++x) {
    (function() { 
AssertType((function() { return x + y}), "() => number");
AssertType(function() { return x + y}, "() => number");
AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
return x + y});

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

    (function() { 
AssertType((function() { return x + y}), "() => any");
AssertType(function() { return x + y}, "() => any");
AssertType(x + y, "any");
AssertType(x, "any");
AssertType(y, "any");
return x + y});

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

    (function() { 
AssertType((function() { return x + y}), "() => any");
AssertType(function() { return x + y}, "() => any");
AssertType(x + y, "any");
AssertType(x, "any");
AssertType(y, "any");
return x + y});

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

    (function() { 
AssertType((function() { return x + y}), "() => number");
AssertType(function() { return x + y}, "() => number");
AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
return x + y});

    (() => x + y);
AssertType((() => x + y), "() => number");
AssertType(() => x + y, "() => number");
AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
}

//=========const
for (const x in {}) {
    (function() { 
AssertType((function() { return x}), "() => string");
AssertType(function() { return x}, "() => string");
AssertType(x, "string");
return x});

    (() => x);
AssertType((() => x), "() => string");
AssertType(() => x, "() => string");
AssertType(x, "string");
}

for (const x of []) {
    (function() { 
AssertType((function() { return x}), "() => any");
AssertType(function() { return x}, "() => any");
AssertType(x, "any");
return x});

    (() => x);
AssertType((() => x), "() => any");
AssertType(() => x, "() => any");
AssertType(x, "any");
}

for (const x = 0; x < 1;) {
    (function() { 
AssertType((function() { return x}), "() => number");
AssertType(function() { return x}, "() => number");
AssertType(x, "int");
return x});

    (() => x);
AssertType((() => x), "() => number");
AssertType(() => x, "() => number");
AssertType(x, "int");
}

while (1 === 1) {
    const x = 1;
AssertType(x, "int");
AssertType(1, "int");

    (function() { 
AssertType((function() { return x}), "() => number");
AssertType(function() { return x}, "() => number");
AssertType(x, "int");
return x});

    (() => x);
AssertType((() => x), "() => number");
AssertType(() => x, "() => number");
AssertType(x, "int");
}

do {
    const x = 1;
AssertType(x, "int");
AssertType(1, "int");

    (function() { 
AssertType((function() { return x}), "() => number");
AssertType(function() { return x}, "() => number");
AssertType(x, "int");
return x});

    (() => x);
AssertType((() => x), "() => number");
AssertType(() => x, "() => number");
AssertType(x, "int");

} while (1 === 1)

for (const y = 0; y < 1;) {
    const x = 1;
AssertType(x, "int");
AssertType(1, "int");

    (function() { 
AssertType((function() { return x}), "() => number");
AssertType(function() { return x}, "() => number");
AssertType(x, "int");
return x});

    (() => x);
AssertType((() => x), "() => number");
AssertType(() => x, "() => number");
AssertType(x, "int");
}

for (const x = 0, y = 1; x < 1;) {
    (function() { 
AssertType((function() { return x + y}), "() => number");
AssertType(function() { return x + y}, "() => number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
return x + y});

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

    (function() { 
AssertType((function() { return x + y}), "() => number");
AssertType(function() { return x + y}, "() => number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
return x + y});

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

    (function() { 
AssertType((function() { return x + y}), "() => number");
AssertType(function() { return x + y}, "() => number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
return x + y});

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

    (function() { 
AssertType((function() { return x + y}), "() => number");
AssertType(function() { return x + y}, "() => number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
return x + y});

    (() => x + y);
AssertType((() => x + y), "() => number");
AssertType(() => x + y, "() => number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
}

