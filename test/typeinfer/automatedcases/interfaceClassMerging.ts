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

// === tests/cases/compiler/interfaceClassMerging.ts ===
declare function AssertType(value:any, type:string):void;
interface Foo {
    method(a: number): string;
    optionalMethod?(a: number): string;
    property: string;
    optionalProperty?: string;
}

class Foo {
    additionalProperty: string;

    additionalMethod(a: number): string {
AssertType(this.method(0), "string");
AssertType(this.method, "(number) => string");
AssertType(this, "this");
AssertType(0, "int");
        return this.method(0);
    }
}

class Bar extends Foo {
    method(a: number) {
AssertType(this.optionalProperty, "string");
AssertType(this, "this");
        return this.optionalProperty;
    }
}


let bar = new Bar();
AssertType(bar, "Bar");
AssertType(new Bar(), "Bar");
AssertType(Bar, "typeof Bar");

bar.method(0);
AssertType(bar.method(0), "string");
AssertType(bar.method, "(number) => string");
AssertType(0, "int");

bar.optionalMethod(1);
AssertType(bar.optionalMethod(1), "string");
AssertType(bar.optionalMethod, "(number) => string");
AssertType(1, "int");

bar.property;
AssertType(bar.property, "string");

bar.optionalProperty;
AssertType(bar.optionalProperty, "string");

bar.additionalProperty;
AssertType(bar.additionalProperty, "string");

bar.additionalMethod(2);
AssertType(bar.additionalMethod(2), "string");
AssertType(bar.additionalMethod, "(number) => string");
AssertType(2, "int");

let obj: {
AssertType(obj, "{ method(number): string; property: string; additionalProperty: string; additionalMethod(number): string; }");

    method(a: number): string;
AssertType(method, "(number) => string");
AssertType(a, "number");

    property: string;
AssertType(property, "string");

    additionalProperty: string;
AssertType(additionalProperty, "string");

    additionalMethod(a: number): string;
AssertType(additionalMethod, "(number) => string");
AssertType(a, "number");

};

bar = obj;
AssertType(bar = obj, "{ method(number): string; property: string; additionalProperty: string; additionalMethod(number): string; }");
AssertType(bar, "Bar");
AssertType(obj, "{ method(number): string; property: string; additionalProperty: string; additionalMethod(number): string; }");

obj = bar;
AssertType(obj = bar, "Bar");
AssertType(obj, "{ method(number): string; property: string; additionalProperty: string; additionalMethod(number): string; }");
AssertType(bar, "Bar");


