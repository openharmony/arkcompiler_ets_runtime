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
let b = 1;
let c = {valueOf : "abc"};

function visit(obj) {
    return obj.valueOf;
}

for (let i = 0; i < 1000; i++) {
    visit(a);
    visit(b);
    visit(c);
}
ArkTools.jitCompileAsync(visit);
print(ArkTools.waitJitCompileFinish(visit));
print(visit(a));
print(visit(b));
print(visit(c));
