/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

declare function print(str: number):string;

function forLoop(n: number): number {
    let sum = 0;
    for (let i = 0; i < n; i++) {
        sum++;
    }
    return sum;
}

let ret = forLoop(2000);
ArkTools.jitCompileAsync(forLoop);
let res = ArkTools.waitJitCompileFinish(forLoop);
print(res);
ret = forLoop(2000);
print(ret);