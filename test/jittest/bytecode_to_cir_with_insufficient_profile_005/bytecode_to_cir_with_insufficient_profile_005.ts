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

function Test(a, b)
{
    // CREATEOBJECTWITHBUFFER_IMM8_ID16
    const testObj = {
        value: 'TestObject',
        Print: function() {
            print(this.value);
        }
    };
    testObj.Print();

    // CREATEARRAYWITHBUFFER_IMM8_ID16
    var arrayObj = [1, 2, 3, 4, 5];
    // GETITERATOR_IMM8
    for (const iter of arrayObj) {
        print(iter);
    }

    // CREATEEMPTYARRAY_IMM8
    var emptyArrayObj = [];
    emptyArrayObj.push(6);
    print(emptyArrayObj);

    // Make the slotid exceed 256.
    // NEWOBJRANGE_IMM8_IMM8_V8
    var paddingArray = new Array(256);
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
    paddingArray[49] += 1;

    // CREATEOBJECTWITHBUFFER_IMM16_ID16
    const testObjTwo = {
        value: 'TestObject',
        Print: function() {
            print(this.value);
        }
    };
    testObjTwo.Print();

    // CREATEARRAYWITHBUFFER_IMM16_ID16
    var arrayObjTwo = [1, 2, 3, 4, 5];
    // GETITERATOR_IMM16
    for (const iter of arrayObjTwo) {
        print(iter);
    }

    // CREATEEMPTYARRAY_IMM16
    var emptyArrayObjTwo = [];
    emptyArrayObjTwo.push(6);
    print(emptyArrayObj);

    // NEWOBJRANGE_IMM16_IMM8_V8
    var paddingArrayTwo = new Array(256);
}

Test();

ArkTools.jitCompileAsync(Test);
print(ArkTools.waitJitCompileFinish(Test));

Test();
