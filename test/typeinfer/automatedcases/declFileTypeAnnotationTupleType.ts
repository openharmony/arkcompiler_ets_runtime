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

// === tests/cases/compiler/declFileTypeAnnotationTupleType.ts ===
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
let k: [c, m.c] = [new c(), new m.c()];
AssertType(k, "[c, m.c]");
AssertType(m, "any");
AssertType([new c(), new m.c()], "[c, m.c]");
AssertType(new c(), "c");
AssertType(c, "typeof c");
AssertType(new m.c(), "m.c");
AssertType(m.c, "typeof m.c");

let l = k;
AssertType(l, "[c, m.c]");
AssertType(k, "[c, m.c]");

let x: [g<string>, m.g<number>, () => c] = [new g<string>(), new m.g<number>(), () => new c()];
AssertType(x, "[g<string>, m.g<number>, () => c]");
AssertType(m, "any");
AssertType([new g<string>(), new m.g<number>(), () => new c()], "[g<string>, m.g<number>, () => c]");
AssertType(new g<string>(), "g<string>");
AssertType(g, "typeof g");
AssertType(new m.g<number>(), "m.g<number>");
AssertType(m.g, "typeof m.g");
AssertType(() => new c(), "() => c");
AssertType(new c(), "c");
AssertType(c, "typeof c");

let y = x;
AssertType(y, "[g<string>, m.g<number>, () => c]");
AssertType(x, "[g<string>, m.g<number>, () => c]");


