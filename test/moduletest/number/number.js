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

// toExponential
function testToExponential(a, b) {
    print(a.toExponential(b));
}

function testToExponentialP(a, b) {
    print(a.toExponential(b));
}

testToExponential(1);
testToExponential(11);
testToExponential(112);
testToExponential(0.1);
testToExponential(0.11);
testToExponential(0.112);
testToExponential(-1);
testToExponential(-11);
testToExponential(-112);
testToExponential(-0.1);
testToExponential(-0.11);
testToExponential(-0.112);
testToExponential(0);
testToExponential(0.000112356);
testToExponential(-0.000112356);

testToExponentialP((1), (1));
testToExponentialP((11), (1));
testToExponentialP((1), (2));
testToExponentialP((112), (2));
testToExponentialP((11), (3));
testToExponentialP((0.1), (1));
testToExponentialP((0.11), (1));
testToExponentialP((0.112), (1));
testToExponentialP((0.112), (2));
testToExponentialP((0.11), (3));
testToExponentialP((-11), (0));
testToExponentialP((-112), (0));
testToExponentialP((-1), (1));
testToExponentialP((-11), (1));
testToExponentialP((-1), (2));
testToExponentialP((-112), (2));
testToExponentialP((-11), (3));
testToExponentialP((-0.1), (1));
testToExponentialP((-0.11), (1));
testToExponentialP((-0.112), (1));
testToExponentialP((-0.112), (2));
testToExponentialP((-0.11), (3));

testToExponentialP((0), (2));
testToExponentialP((11.2356), (0));
testToExponentialP((11.2356), (4));
testToExponentialP((0.000112356), (4));
testToExponentialP((-0.000112356), (4));


// toPrecision
function testToPrecision(a, b) {
    print(a.toPrecision(b));
}
testToPrecision((0.000555), (15));
testToPrecision((0.000000555), (15));
testToPrecision((-0.000000555), (15));
testToPrecision((123456789), (1));
testToPrecision((123456789), (9));
testToPrecision((123456789), (8));
testToPrecision((123456789), (7));
testToPrecision((-123456789), (7));
testToPrecision(Number(-.0000000012345), (2));
testToPrecision(Number(-.000012345), (2));
testToPrecision(Number(-.00012345), (2));
testToPrecision(Number(-1.2345), (2));
testToPrecision(Number(-12.345), (2));
testToPrecision(Number(-1234.5), (2));
testToPrecision(Number(-12345.67), (4));


// toString, Only Test InttoString
function testToString(a, b) {
    print(a.toString(b));
}
print((-0).toString());
print((1).toString());
print((-9).toString());
print((1234567).toString());
print((-1234567).toString());
print((10000).toString());
print((67345).toString());

testToString((0), (16));
testToString((9), (16));
testToString((90), (16));
testToString((-0), (16));
testToString((-90), (16));
testToString((10000007), (36));
testToString((123456789), (32));
testToString((-123456789), (32));

var result = "";
var num_1 = { valueOf: function() { result += "first"; return 1; } };
var num_2 = { valueOf: function() { result += "second"; return 2; } };
// Compare the size of number 1 and number 2
num_1 < num_2
print("firstsecond" == result);

result = "";
num_1 = { valueOf: function() { result += "first"; return 1; } };
// Compare the size of number 1 and 0
num_1 > void(0);
print("first" == result);

result = "";
num_2 = { valueOf: function() { result += "second"; return 2; } };
// Compare the size of 0 and number 2
void(0) > num_2;
print("second" == result);
