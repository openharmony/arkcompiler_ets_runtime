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

// === tests/cases/compiler/declFileTypeAnnotationTypeQuery.ts ===
declare function AssertType(value:any, type:string):void;
class c {
}
module m {
    export class c {
    }
    export class g<T> {
    }
}
class g<T> {
}

// Just the name
function foo(): typeof c {
AssertType(c, "typeof c");
    return c;
}
function foo2() {
AssertType(c, "typeof c");
    return c;
}

// Qualified name
function foo3(): typeof m.c {
AssertType(m.c, "typeof m.c");
    return m.c;
}
function foo4() {
AssertType(m.c, "typeof m.c");
    return m.c;
}

// Just the name with type arguments
function foo5(): typeof g {
AssertType(g, "typeof g");
    return g;
}
function foo6() {
AssertType(g, "typeof g");
    return g;
}

// Qualified name with type arguments
function foo7(): typeof m.g {
AssertType(m.g, "typeof m.g");
    return m.g
}
function foo8() {
AssertType(m.g, "typeof m.g");
    return m.g
}

