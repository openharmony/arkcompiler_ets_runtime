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

// === tests/cases/compiler/targetTypeCalls.ts ===
declare function AssertType(value:any, type:string):void;
let fra1: (v:any)=>string = function() { 
AssertType(fra1, "(any) => string");
AssertType(v, "any");
AssertType(function() { return function (v:string) {return v;}; }(), "(string) => string");
AssertType(function() { return function (v:string) {return v;}; }, "() => (string) => string");
AssertType(function (v:string) {return v;}, "(string) => string");
AssertType(v, "string");
AssertType(v, "string");
return function (v:string) {return v;}; }() // should work

let fra2: (v:any)=>number = function() { 
AssertType(fra2, "(any) => number");
AssertType(v, "any");
AssertType(function() { return function () { return 0; } }(), "() => number");
AssertType(function() { return function () { return 0; } }, "() => () => number");
AssertType(function () { return 0; }, "() => number");
AssertType(0, "int");
return function () { return 0; } }() // should work

let fra3: (v:any)=>string = function() { 
AssertType(fra3, "(any) => string");
AssertType(v, "any");
AssertType(function() { return function() { return function(v) {return v;};}(); }(), "(any) => any");
AssertType(function() { return function() { return function(v) {return v;};}(); }, "() => (any) => any");
AssertType(function() { return function(v) {return v;};}(), "(any) => any");
AssertType(function() { return function(v) {return v;};}, "() => (any) => any");
AssertType(function(v) {return v;}, "(any) => any");
AssertType(v, "any");
AssertType(v, "any");
return function() { return function(v) {return v;};}(); }() // should work

let fra4: (v:any)=>void = function() { 
AssertType(fra4, "(any) => void");
AssertType(v, "any");
AssertType(function() { return function() { return function(v) {return v;};}(); }(), "(any) => any");
AssertType(function() { return function() { return function(v) {return v;};}(); }, "() => (any) => any");
AssertType(function() { return function(v) {return v;};}(), "(any) => any");
AssertType(function() { return function(v) {return v;};}, "() => (any) => any");
AssertType(function(v) {return v;}, "(any) => any");
AssertType(v, "any");
AssertType(v, "any");
return function() { return function(v) {return v;};}(); }() // should work


