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

// === tests/cases/compiler/extendedUnicodePlaneIdentifiers.ts ===
declare function AssertType(value:any, type:string):void;
const 𝑚 = 4;
AssertType(𝑚, "int");
AssertType(4, "int");

const 𝑀 = 5;
AssertType(𝑀, "int");
AssertType(5, "int");

console.log(𝑀 + 𝑚); // 9
AssertType(console.log(𝑀 + 𝑚), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(𝑀 + 𝑚, "number");
AssertType(𝑀, "int");
AssertType(𝑚, "int");

class K {
    #𝑚 = 4;
    #𝑀 = 5;
}

// lower 8 bits look like 'a'
const ၡ = 6;
AssertType(ၡ, "int");
AssertType(6, "int");

console.log(ၡ ** ၡ);
AssertType(console.log(ၡ ** ၡ), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(ၡ ** ၡ, "number");
AssertType(ၡ, "int");
AssertType(ၡ, "int");

// lower 8 bits aren't a valid unicode character
const ဒ = 7;
AssertType(ဒ, "int");
AssertType(7, "int");

console.log(ဒ ** ဒ);
AssertType(console.log(ဒ ** ဒ), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(ဒ ** ဒ, "number");
AssertType(ဒ, "int");
AssertType(ဒ, "int");

// a mix, for good measure
const ဒၡ𝑀 = 7;
AssertType(ဒၡ𝑀, "int");
AssertType(7, "int");

console.log(ဒၡ𝑀 ** ဒၡ𝑀);
AssertType(console.log(ဒၡ𝑀 ** ဒၡ𝑀), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(ဒၡ𝑀 ** ဒၡ𝑀, "number");
AssertType(ဒၡ𝑀, "int");
AssertType(ဒၡ𝑀, "int");

const ၡ𝑀ဒ = 7;
AssertType(ၡ𝑀ဒ, "int");
AssertType(7, "int");

console.log(ၡ𝑀ဒ ** ၡ𝑀ဒ);
AssertType(console.log(ၡ𝑀ဒ ** ၡ𝑀ဒ), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(ၡ𝑀ဒ ** ၡ𝑀ဒ, "number");
AssertType(ၡ𝑀ဒ, "int");
AssertType(ၡ𝑀ဒ, "int");

const 𝑀ဒၡ = 7;
AssertType(𝑀ဒၡ, "int");
AssertType(7, "int");

console.log(𝑀ဒၡ ** 𝑀ဒၡ);
AssertType(console.log(𝑀ဒၡ ** 𝑀ဒၡ), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(𝑀ဒၡ ** 𝑀ဒၡ, "number");
AssertType(𝑀ဒၡ, "int");
AssertType(𝑀ဒၡ, "int");

const 𝓱𝓮𝓵𝓵𝓸 = "𝔀𝓸𝓻𝓵𝓭";
AssertType(𝓱𝓮𝓵𝓵𝓸, "string");
AssertType("𝔀𝓸𝓻𝓵𝓭", "string");

const Ɐⱱ = "ok"; // BMP
AssertType(Ɐⱱ, "string");
AssertType("ok", "string");

const 𓀸𓀹𓀺 = "ok"; // SMP
AssertType(𓀸𓀹𓀺, "string");
AssertType("ok", "string");

const 𡚭𡚮𡚯 = "ok"; // SIP
AssertType(𡚭𡚮𡚯, "string");
AssertType("ok", "string");

const 𡚭𓀺ⱱ𝓮 = "ok";
AssertType(𡚭𓀺ⱱ𝓮, "string");
AssertType("ok", "string");

const 𓀺ⱱ𝓮𡚭 = "ok";
AssertType(𓀺ⱱ𝓮𡚭, "string");
AssertType("ok", "string");

const ⱱ𝓮𡚭𓀺 = "ok";
AssertType(ⱱ𝓮𡚭𓀺, "string");
AssertType("ok", "string");

const 𝓮𡚭𓀺ⱱ = "ok";
AssertType(𝓮𡚭𓀺ⱱ, "string");
AssertType("ok", "string");


