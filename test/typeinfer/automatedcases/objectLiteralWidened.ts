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

// === tests/cases/conformance/types/typeRelationships/widenedTypes/objectLiteralWidened.ts ===
declare function AssertType(value:any, type:string):void;
// object literal properties are widened to any

let x1 = {
AssertType(x1, "{ foo: any; bar: any; }");
AssertType({    foo: null,    bar: undefined}, "{ foo: null; bar: undefined; }");

    foo: null,
AssertType(foo, "null");
AssertType(null, "null");

    bar: undefined
AssertType(bar, "undefined");
AssertType(undefined, "undefined");
}

let y1 = {
AssertType(y1, "{ foo: any; bar: { baz: any; boo: any; }; }");
AssertType({    foo: null,    bar: {        baz: null,        boo: undefined    }}, "{ foo: null; bar: { baz: null; boo: undefined; }; }");

    foo: null,
AssertType(foo, "null");
AssertType(null, "null");

    bar: {
AssertType(bar, "{ baz: null; boo: undefined; }");
AssertType({        baz: null,        boo: undefined    }, "{ baz: null; boo: undefined; }");

        baz: null,
AssertType(baz, "null");
AssertType(null, "null");

        boo: undefined
AssertType(boo, "undefined");
AssertType(undefined, "undefined");
    }
}

// these are not widened

let u: undefined = undefined;
AssertType(u, "undefined");
AssertType(undefined, "undefined");

let n: null = null;
AssertType(n, "null");
AssertType(null, "null");
AssertType(null, "null");

let x2 = {
AssertType(x2, "{ foo: null; bar: undefined; }");
AssertType({    foo: n,    bar: u}, "{ foo: null; bar: undefined; }");

    foo: n,
AssertType(foo, "null");
AssertType(n, "null");

    bar: u
AssertType(bar, "undefined");
AssertType(u, "undefined");
}

let y2 = {
AssertType(y2, "{ foo: null; bar: { baz: null; boo: undefined; }; }");
AssertType({    foo: n,    bar: {        baz: n,        boo: u    }}, "{ foo: null; bar: { baz: null; boo: undefined; }; }");

    foo: n,
AssertType(foo, "null");
AssertType(n, "null");

    bar: {
AssertType(bar, "{ baz: null; boo: undefined; }");
AssertType({        baz: n,        boo: u    }, "{ baz: null; boo: undefined; }");

        baz: n,
AssertType(baz, "null");
AssertType(n, "null");

        boo: u
AssertType(boo, "undefined");
AssertType(u, "undefined");
    }
}

