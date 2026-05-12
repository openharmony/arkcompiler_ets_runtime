/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
function helper1(x) {
    return x * 2;
}

function helper2(a, b) {
    return a + b;
}

function factorial(n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

function function_call_test()
{
    let result = 0;

    // direct function call
    result = helper1(5);

    // function call with multiple arguments
    result = helper2(result, 10);

    // recursive function call
    result = factorial(5);

    // method call on object
    let obj = {
        value: 10,
        getValue: function() {
            return this.value;
        },
        add: function(x) {
            return this.value + x;
        }
    };
    result = obj.getValue();
    result = obj.add(5);

    // built-in function calls
    let str = "Hello";
    result = str.length;
    let arr = [1, 2, 3];
    result = arr.push(4); // method call that returns new length

    // function call in expression
    result = helper1(helper2(3, 4));

    return result;
}

ArkTools.arkSteedCompileAsync(function_call_test);

// Spin loop: A temporary approach to wait for ArkSteed JIT compiler
let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

// TODO: Replace the spin loop above to:
// let res = ArkTools.waitJitCompileFinish(function_call_test);
// print(res);

let output = function_call_test();
print(output);