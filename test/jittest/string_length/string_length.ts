/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

function f(arg) {
    let v = arg[0];
    let c = v.charCodeAt(0);
    return v.length > 0 ? c : ".";
}

let ss = "abcdefg"
for (let i = 0; i < 20; i++) {
    f(ss)
}

ArkTools.jitCompileAsync(f);
print(ArkTools.waitJitCompileFinish(f))

print(f(ss))

function f2() {
    let v1 = 0, v18 = 0, v28 = 1;
    for (; v28--;) {
        ({ "length": v18, ...v1 } = "123456789");
    }
    return [v18, v1];
}

f2();
ArkTools.jitCompileAsync(f2);
print(ArkTools.waitJitCompileFinish(f2))

print(f2())
