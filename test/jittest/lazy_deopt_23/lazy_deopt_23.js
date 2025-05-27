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

function f(arr) {
    print(arr.concat);
}

let arr = []
f(arr);
ArkTools.jitCompileAsync(f);
print(ArkTools.waitJitCompileFinish(f));

print("------------------------------------------------------");

arr[1] = 1;
arr[1000] = 1;
f(arr);

arr[500] = "233";
f(arr);

let arr2 = []
arr2[2000] = 1;
f(arr2);