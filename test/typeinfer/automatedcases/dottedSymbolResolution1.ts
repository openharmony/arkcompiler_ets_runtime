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

// === tests/cases/compiler/dottedSymbolResolution1.ts ===
declare function AssertType(value:any, type:string):void;
interface JQuery {
    find(selector: string): JQuery;
}

interface JQueryStatic {
    
    (selector: string): JQuery;
    (object: JQuery): JQuery;
}

class Base { foo() { } }

function each(collection: string, callback: (indexInArray: any, valueOfElement: any) => any): any;
function each(collection: JQuery, callback: (indexInArray: number, valueOfElement: Base) => any): any;
function each(collection: any, callback: (indexInArray: any, valueOfElement: any) => any): any {
AssertType(null, "null");
    return null;
}

function _setBarAndText(): void {
    let x: JQuery, $: JQueryStatic
AssertType(x, "JQuery");
AssertType($, "JQueryStatic");

    each(x.find(" "), function () {
AssertType(each(x.find(" "), function () {        let $this: JQuery = $(''),            thisBar = $this.find(".fx-usagebars-calloutbar-this"); // bug lead to 'could not find dotted symbol' here    } ), "any");
AssertType(each, "{ (string, (any, any) => any): any; (JQuery, (number, Base) => any): any; }");
AssertType(x.find(" "), "JQuery");
AssertType(x.find, "(string) => JQuery");
AssertType(" ", "string");
AssertType(function () {        let $this: JQuery = $(''),            thisBar = $this.find(".fx-usagebars-calloutbar-this"); // bug lead to 'could not find dotted symbol' here    }, "() => void");

        let $this: JQuery = $(''),
AssertType($this, "JQuery");
AssertType($(''), "JQuery");
AssertType($, "JQueryStatic");
AssertType('', "string");

            thisBar = $this.find(".fx-usagebars-calloutbar-this"); // bug lead to 'could not find dotted symbol' here
AssertType(thisBar, "JQuery");
AssertType($this.find(".fx-usagebars-calloutbar-this"), "JQuery");
AssertType($this.find, "(string) => JQuery");
AssertType(".fx-usagebars-calloutbar-this", "string");

    } );
}

