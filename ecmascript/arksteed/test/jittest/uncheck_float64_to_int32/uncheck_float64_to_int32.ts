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

// Test scenario: No deopt check when storing a float value to an int typed array.
function f1(x) {
    x = x * 1.1;
    let arr = new Int32Array(1);
    arr[0] = x;
    print(arr[0]);
}

f1(1.1);
ArkTools.arkSteedCompileAsync(f1);
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
f1(2.2);

// Test scenario: No deopt check when performing logic shift on a float value.
function f2(x) {
    x = x * 1.1;
    let left = x << 1;
    let right = x >> 1;
    print("left shift result: ", left, " right shift result: ", right);
}

f2(1.1);
ArkTools.arkSteedCompileAsync(f2);
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
f2(2.2);

// Test scenario: No deopt check when performing not on a float value.
function f3(x) {
    x = x * 1.1;
    let y = ~x;
    print(y);
}

f3(1.1);
ArkTools.arkSteedCompileAsync(f3);
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
f3(2.2);

// Test scenario: No deopt check when performing conversions between int32 and float64.
function f4(x) {
    if (x++ < -1) {
        x++;  
    }
    return x
}

f4(-200);
f4(12345678912345);
ArkTools.arkSteedCompileAsync(f4);
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print(f4(-12345678912345) == -12345678912343);
