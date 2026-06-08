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

// Fibonacci with closure-based memoization
let compiled = false;

function call_compiled_lambda_11(n: number): number {
    let memo: number[] = [0, 1];
    const fib = function(k: number): number {
        if (memo[k] != undefined) {
            return memo[k];
        }
        let a: number = 0;
        let b: number = 1;
        for (let i: number = 2; i <= k; i = i + 1) {
            let next: number = a + b;
            a = b;
            b = next;
            memo[i] = b;
        }
        return b;
    };
    if (!compiled) {
        ArkTools.arkSteedCompileSync(fib);
        compiled = true;
    }
    return fib(n);
}

ArkTools.arkSteedCompileSync(call_compiled_lambda_11);
print(call_compiled_lambda_11(0));
print(call_compiled_lambda_11(1));
print(call_compiled_lambda_11(5));
print(call_compiled_lambda_11(10));
print(call_compiled_lambda_11(15));
