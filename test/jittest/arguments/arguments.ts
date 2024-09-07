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

let f = function(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9) {
    let sum = 0;
    for (let i = 0; i < arguments.length; i++) {
        sum = sum + arguments[i]
    }
    print(sum)
}

f(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13)

function printArgs(...rest:any)
{
    print(arguments[0]);
    print(arguments[1]);
}

printArgs("aaa", 55555);

ArkTools.jitCompileAsync(f);
ArkTools.jitCompileAsync(printArgs);
ArkTools.waitJitCompileFinish(printArgs);
let res = ArkTools.waitJitCompileFinish(f);
f(0, 1, 2)
printArgs("aaa", 55555);
print(res)
