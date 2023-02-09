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

// === tests/cases/conformance/es2021/logicalAssignment/logicalAssignment10.ts ===
declare function AssertType(value:any, type:string):void;
let count = 0;
AssertType(count, "number");
AssertType(0, "int");

let obj = {};
AssertType(obj, "{}");
AssertType({}, "{}");

function incr() {
AssertType(++count, "number");
AssertType(count, "number");
    return ++count;
}

const oobj = {
AssertType(oobj, "{ obj: {}; }");
AssertType({    obj}, "{ obj: {}; }");

    obj
AssertType(obj, "{}");
}

obj[incr()] ??= incr();
AssertType(obj[incr()] ??= incr(), "any");
AssertType(obj[incr()], "error");
AssertType(obj, "{}");
AssertType(incr(), "number");
AssertType(incr, "() => number");
AssertType(incr(), "number");
AssertType(incr, "() => number");

oobj["obj"][incr()] ??= incr();
AssertType(oobj["obj"][incr()] ??= incr(), "any");
AssertType(oobj["obj"][incr()], "error");
AssertType(oobj["obj"], "{}");
AssertType(oobj, "{ obj: {}; }");
AssertType("obj", "string");
AssertType(incr(), "number");
AssertType(incr, "() => number");
AssertType(incr(), "number");
AssertType(incr, "() => number");


