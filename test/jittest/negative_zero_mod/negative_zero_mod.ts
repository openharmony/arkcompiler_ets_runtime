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

function moduloWithNegativeZeroDividend(a, b, c) {
    var temp = a * b;
    return temp % c;
}

for (var i = 1; i < 100; i++) {
    var result = moduloWithNegativeZeroDividend(i, 5, 5);
    if (result !== 0) {
        throw "moduloWithNegativeZeroDividend(i, 5, 5), returned: " + result;
    }
}

ArkTools.jitCompileAsync(moduloWithNegativeZeroDividend);
var compilerResult = ArkTools.waitJitCompileFinish(moduloWithNegativeZeroDividend);
print("compilerResult: " + compilerResult);

for (var i = 1; i < 100; i++) {
    var result = moduloWithNegativeZeroDividend(-i, 0, 2);
    if (!(result === 0 && (1 / result) === -Infinity)) {
        throw "moduloWithNegativeZeroDividend(i, 5, 5), returned: " + result;
    }
}
