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

// Lambda with loop: accumulate sum of arithmetic sequence
let compiled = false;

function call_compiled_lambda_10(start: number, end: number, step: number): number {
    const sumRange = function(a: number, b: number, s: number): number {
        let total: number = 0;
        for (let i: number = a; i <= b; i = i + s) {
            total = total + i;
        }
        return total;
    };
    if (!compiled) {
        ArkTools.arkSteedCompileSync(sumRange);
        compiled = true;
    }
    return sumRange(start, end, step);
}

ArkTools.arkSteedCompileSync(call_compiled_lambda_10);
print(call_compiled_lambda_10(1, 5, 1));
print(call_compiled_lambda_10(2, 10, 2));
print(call_compiled_lambda_10(0, 10, 5));
