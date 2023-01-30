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

// === tests/cases/compiler/genericMethodOverspecialization.ts ===
declare function AssertType(value:any, type:string):void;
let names = ["list", "table1", "table2", "table3", "summary"];
AssertType(names, "string[]");
AssertType(["list", "table1", "table2", "table3", "summary"], "string[]");
AssertType("list", "string");
AssertType("table1", "string");
AssertType("table2", "string");
AssertType("table3", "string");
AssertType("summary", "string");

interface HTMLElement {
    clientWidth: number;
    isDisabled: boolean;
}

declare let document: Document;
AssertType(document, "Document");

interface Document {
    getElementById(elementId: string): HTMLElement;
}

let elements = names.map(function (name) {
AssertType(elements, "HTMLElement[]");
AssertType(names.map, "<U>((string, number, string[]) => U, ?any) => U[]");
AssertType(function (name) {    return document.getElementById(name);}, "(string) => HTMLElement");
AssertType(name, "string");
AssertType(names.map(function (name) {    return document.getElementById(name);}), "HTMLElement[]");

AssertType(document.getElementById(name), "HTMLElement");
AssertType(document.getElementById, "{ (string): HTMLElement; (string): HTMLElement; }");
AssertType(name, "string");
    return document.getElementById(name);

});


let xxx = elements.filter(function (e) {
AssertType(xxx, "HTMLElement[]");
AssertType(elements.filter, "{ <S extends HTMLElement>((HTMLElement, number, HTMLElement[]) => value is S, ?any): S[]; ((HTMLElement, number, HTMLElement[]) => unknown, ?any): HTMLElement[]; }");
AssertType(function (e) {    return !e.isDisabled;}, "(HTMLElement) => boolean");
AssertType(e, "HTMLElement");
AssertType(elements.filter(function (e) {    return !e.isDisabled;}), "HTMLElement[]");

AssertType(!e.isDisabled, "boolean");
AssertType(e.isDisabled, "boolean");
    return !e.isDisabled;

});

let widths:number[] = elements.map(function (e) { // should not error
AssertType(widths, "number[]");
AssertType(elements.map, "<U>((HTMLElement, number, HTMLElement[]) => U, ?any) => U[]");
AssertType(function (e) { // should not error    return e.clientWidth;}, "(HTMLElement) => number");
AssertType(e, "HTMLElement");
AssertType(elements.map(function (e) { // should not error    return e.clientWidth;}), "number[]");

AssertType(e.clientWidth, "number");
    return e.clientWidth;

});



