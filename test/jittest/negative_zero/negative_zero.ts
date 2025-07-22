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

function testNegativeZeroMod(a, b, c) {
    var temp = a * b;
    return temp % c;
}

function testNegativeZeroMul(a, b) {
    var temp = a * b;
    return temp;
}

function testNegativeZeroDiv(a, b, c) {
    var temp = a * b;
    return temp / c;
}

for (var i = 1; i < 100; i++) {
    var result = 0;
    result = testNegativeZeroMod(i, 5, 5);
    if (result !== 0) {
        throw "testNegativeZeroMod(i, 5, 5), returned: " + result;
    }
    result = testNegativeZeroMul(i, 0);
    if (result !== 0) {
        throw "testNegativeZeroMul(i, 0), returned: " + result;
    }
    result = testNegativeZeroDiv(0, i, i);
    if (result !== 0) {
        throw "testNegativeZeroDiv(0, i, i), returned: " + result;
    }
}

ArkTools.jitCompileAsync(testNegativeZeroMod);
ArkTools.jitCompileAsync(testNegativeZeroMul);
ArkTools.jitCompileAsync(testNegativeZeroDiv);
print(ArkTools.waitJitCompileFinish(testNegativeZeroMod));
print(ArkTools.waitJitCompileFinish(testNegativeZeroMul));
print(ArkTools.waitJitCompileFinish(testNegativeZeroDiv));

for (var i = 1; i < 100; i++) {
    var result = 0;
    result = testNegativeZeroMod(-i, 0, 2);
    if (!(result === 0 && (1 / result) === -Infinity)) {
        throw "testNegativeZeroMod(-i, 0, 2), returned: " + result;
    }
    result = testNegativeZeroMul(-i, 0);
    if (!(result === 0 && (1 / result) === -Infinity)) {
        throw "testNegativeZeroMul(-i, 0), returned: " + result;
    }
    result = testNegativeZeroDiv(0, -i, i);
    if (!(result === 0 && (1 / result) === -Infinity)) {
        throw "testNegativeZeroDiv(0, -i, i), returned: " + result;
    }
}
