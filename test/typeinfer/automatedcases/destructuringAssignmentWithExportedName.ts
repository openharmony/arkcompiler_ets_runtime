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

// === tests/cases/compiler/destructuringAssignmentWithExportedName.ts ===
declare function AssertType(value:any, type:string):void;
export let exportedFoo: any;
AssertType(exportedFoo, "any");

let nonexportedFoo: any;
AssertType(nonexportedFoo, "any");

// sanity checks
exportedFoo = null;
AssertType(exportedFoo = null, "null");
AssertType(exportedFoo, "any");
AssertType(null, "null");

nonexportedFoo = null;
AssertType(nonexportedFoo = null, "null");
AssertType(nonexportedFoo, "any");
AssertType(null, "null");

if (null as any) {
    ({ exportedFoo, nonexportedFoo } = null as any);
AssertType(({ exportedFoo, nonexportedFoo } = null as any), "any");
AssertType({ exportedFoo, nonexportedFoo } = null as any, "any");
AssertType({ exportedFoo, nonexportedFoo }, "{ exportedFoo: any; nonexportedFoo: any; }");
AssertType(exportedFoo, "any");
AssertType(nonexportedFoo, "any");
AssertType(null as any, "any");
AssertType(null, "null");
}
else if (null as any) {
	({ foo: exportedFoo, bar: nonexportedFoo } = null as any);
AssertType(({ foo: exportedFoo, bar: nonexportedFoo } = null as any), "any");
AssertType({ foo: exportedFoo, bar: nonexportedFoo } = null as any, "any");
AssertType({ foo: exportedFoo, bar: nonexportedFoo }, "{ foo: any; bar: any; }");
AssertType(foo, "any");
AssertType(exportedFoo, "any");
AssertType(bar, "any");
AssertType(nonexportedFoo, "any");
AssertType(null as any, "any");
AssertType(null, "null");
}
else if (null as any) {
	({ foo: { bar: exportedFoo, baz: nonexportedFoo } } = null as any);
AssertType(({ foo: { bar: exportedFoo, baz: nonexportedFoo } } = null as any), "any");
AssertType({ foo: { bar: exportedFoo, baz: nonexportedFoo } } = null as any, "any");
AssertType({ foo: { bar: exportedFoo, baz: nonexportedFoo } }, "{ foo: { bar: any; baz: any; }; }");
AssertType(foo, "{ bar: any; baz: any; }");
AssertType({ bar: exportedFoo, baz: nonexportedFoo }, "{ bar: any; baz: any; }");
AssertType(bar, "any");
AssertType(exportedFoo, "any");
AssertType(baz, "any");
AssertType(nonexportedFoo, "any");
AssertType(null as any, "any");
AssertType(null, "null");
}
else if (null as any) {
	([exportedFoo, nonexportedFoo] = null as any);
AssertType(([exportedFoo, nonexportedFoo] = null as any), "any");
AssertType([exportedFoo, nonexportedFoo] = null as any, "any");
AssertType([exportedFoo, nonexportedFoo], "[any, any]");
AssertType(exportedFoo, "any");
AssertType(nonexportedFoo, "any");
AssertType(null as any, "any");
AssertType(null, "null");
}
else {
	([[exportedFoo, nonexportedFoo]] = null as any);
AssertType(([[exportedFoo, nonexportedFoo]] = null as any), "any");
AssertType([[exportedFoo, nonexportedFoo]] = null as any, "any");
AssertType([[exportedFoo, nonexportedFoo]], "[[any, any]]");
AssertType([exportedFoo, nonexportedFoo], "[any, any]");
AssertType(exportedFoo, "any");
AssertType(nonexportedFoo, "any");
AssertType(null as any, "any");
AssertType(null, "null");
}

export { nonexportedFoo };
export { exportedFoo as foo, nonexportedFoo as nfoo };

