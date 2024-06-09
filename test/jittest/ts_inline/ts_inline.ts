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

function func(arg1 : number, arg2 : number) {
    let a3 = 0;
    let a1 = arg1 + arg2;
    if (a1 > 0) {
        a1 = a1 * 2;
    } else {
        a1++;
    }
    let a2 = arg1 * arg2;
    if (a2 > 0) {
        a2 = a2 / 2;
    } else {
        a2++;
    }
    a3 = a1 + a2;
    return a3;
}

function Test() {
    return func(8, 2);
}

Test();
ArkTools.jitCompileAsync(Test);
let res = ArkTools.waitJitCompileFinish(Test);
print(Test());
print(res);
