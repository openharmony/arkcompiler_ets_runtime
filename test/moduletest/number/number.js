/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

/*
 * @tc.name:Number
 * @tc.desc:test Number
 * @tc.type: FUNC
 * @tc.require: issueI8S2AX
 */

// toFixed
function testToFixed(a, b) {
    print(a.toFixed(b));
}

// toPrecision
function testToPrecision(a, b) {
    print(a.toPrecision(b));
}

// toExponential
function testToExponential(a, b) {
    print(a.toExponential(b));
}

testToFixed(1.25, 1);
testToPrecision(1.25, 2);
print(0.000112356.toExponential())
print((-(true)).toPrecision(0x30, 'lib1-f1'))
testToFixed(1111111111111111111111, 8);
testToFixed(0.1, 1);
testToFixed(0.1, 2);
testToFixed(0.1, 3);
testToFixed(0.01, 2);
testToFixed(0.01, 3);
testToFixed(0.01, 4);
testToFixed(0.0000006, 7);
testToFixed(0.00000006, 8);
testToFixed(0.00000006, 9);
testToFixed(0.00000006, 10);
testToFixed(1.12, 0);
testToFixed(12.12, 0);
testToFixed(12.12, 1);
testToFixed(12.122, 8);
testToFixed(-1111111111111111111111, 8);
testToFixed((-0.1), 1);
testToFixed((-0.1), 2);
testToFixed((-0.1), 3);
testToFixed((-0.01), 2);
testToFixed((-0.01), 3);
testToFixed((-0.01), 4);
testToFixed((-0.0000006), 7);
testToFixed((-0.0000006), 8);
testToFixed((-0.0000006), 9);
testToFixed((-0.0000006), 10);
testToFixed((-1.12), 0);
testToFixed((-12.12), 0);
testToFixed((-12.12), 1);
testToFixed((-12.122), 8);