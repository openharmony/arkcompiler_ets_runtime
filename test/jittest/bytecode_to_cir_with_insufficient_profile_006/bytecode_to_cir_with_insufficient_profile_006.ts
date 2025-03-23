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

class TestClass {
    constructor() {
        this.one = 0;
    }
    Add(left, right) {
        for (var k = 0; k < right; ++k) {
            this.one += k;
        }
        return left + right + this.one;
    }
}

function Test() {
    var testClass = new TestClass();
    var paddingArray = newArray(256);
    // STOBJBYNAME_IMM8_ID16_V8
    testClass.one = 0;
    // LDOBJBYNAME_IMM8_ID16
    // LDOBJBYVALUE_IMM8_V8
    print(testClass.Add(paddingArray[0], paddingArray[1]));
    // STOBJBYVALUE_IMM8_V8_V8
    paddingArray[0] += 1;
    paddingArray[1] += 1;
    paddingArray[2] += 1;
    paddingArray[3] += 1;
    paddingArray[4] += 1;
    paddingArray[5] += 1;
    paddingArray[6] += 1;
    paddingArray[7] += 1;
    paddingArray[8] += 1;
    paddingArray[9] += 1;
    paddingArray[10] += 1;
    paddingArray[11] += 1;
    paddingArray[12] += 1;
    paddingArray[13] += 1;
    paddingArray[14] += 1;
    paddingArray[15] += 1;
    paddingArray[16] += 1;
    paddingArray[17] += 1;
    paddingArray[18] += 1;
    paddingArray[19] += 1;
    paddingArray[20] += 1;
    paddingArray[21] += 1;
    paddingArray[22] += 1;
    paddingArray[23] += 1;
    paddingArray[24] += 1;
    paddingArray[25] += 1;
    paddingArray[26] += 1;
    paddingArray[27] += 1;
    paddingArray[28] += 1;
    paddingArray[29] += 1;
    paddingArray[30] += 1;
    paddingArray[31] += 1;
    paddingArray[32] += 1;
    paddingArray[33] += 1;
    paddingArray[34] += 1;
    paddingArray[35] += 1;
    paddingArray[36] += 1;
    paddingArray[37] += 1;
    paddingArray[38] += 1;
    paddingArray[39] += 1;
    paddingArray[40] += 1;
    paddingArray[41] += 1;
    paddingArray[42] += 1;
    paddingArray[43] += 1;
    paddingArray[44] += 1;
    paddingArray[45] += 1;
    paddingArray[46] += 1;
    paddingArray[47] += 1;
    paddingArray[48] += 1;
    // STOBJBYVALUE_IMM16_V8_V8
    paddingArray[49] += 1;
    // STOBJBYNAME_IMM16_ID16_V8
    testClass.one = 0;
    // LDOBJBYNAME_IMM16_ID16
    // LDOBJBYVALUE_IMM16_V8
    print(testClass.Add(paddingArray[0], paddingArray[1]));
}

Test();

ArkTools.jitCOmpileAsync(Test);
print(ArkTools.waitJitCompileFinish(Test));

Test();
