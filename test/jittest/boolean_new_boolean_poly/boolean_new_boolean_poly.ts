/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 */

// @ts-nocheck
declare function print(arg: any): string;

let a = true;
let b = new Boolean(false);
b.valueOf = "789"

function visit(obj) {
    return obj.valueOf;
}

Boolean.prototype.valueOf = "Boolean.ValueOf"

for (let i = 0; i < 1000; i++) {
    visit(a);
    visit(b);
}
ArkTools.jitCompileAsync(visit);
print(ArkTools.waitJitCompileFinish(visit));
print(visit(a));

delete Boolean.prototype.valueOf;
print(visit(a));

Boolean.prototype.valueOf = "boolean.valueOf"
print(visit(a))

let c = new Boolean(true);
c.valueOf = "1234";
print(visit(c));
