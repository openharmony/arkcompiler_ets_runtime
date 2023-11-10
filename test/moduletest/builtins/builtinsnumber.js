/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
 * @tc.name:builtins
 * @tc.desc:test builtins
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
let result = Number.parseInt("16947500000");
print("builtins number start");
print("parseInt result = " + result);
print(1 / 0.75 * 0.6);
print(1 / (-1 * 0));

print("builtins number parsefloat");
print(Number.parseFloat())
print(Number.parseFloat(123))
print(Number.parseFloat(new Number(-0.12)))
print(Number.parseFloat("10.123"))
print(Number.parseFloat("-0"))
print(Number.parseFloat("0"))
print(Number.parseFloat("-1000, 0"))
print(Number.parseFloat("-1000"))
print(Number.parseFloat("-0.12"))
print(Number.parseFloat("  01 ", "1"))
print(Number.parseFloat("123456e10"))
print(Number.parseFloat("123456e10"))
print(Number.parseFloat("1.0"))
print(Number.parseFloat("1e0"))
print(Number.parseFloat("10.0e-0"))
print(Number.parseFloat("10.0e0"))
print(Number.parseFloat("100.0E1"))

print("builtins global parsefloat");
print(parseFloat())
print(parseFloat(123))
print(parseFloat(new Number(-0.12)))
print(parseFloat("10.123"))
print(parseFloat("-0"))
print(parseFloat("0"))
print(parseFloat("-1000, 0"))
print(parseFloat("-1000"))
print(parseFloat("-0.12"))
print(parseFloat("  01 ", "1"))
print(parseFloat("123456e10"))
print(parseFloat("123456e10"))

print("builtins number tostring");
print((10 ** 21.5).toString())
print((10 ** 21.5).toString())
let n1 = new Number(0)
print(n1.toString())
print(n1.toString())
n1 = new Number(-0)
print(n1.toString())
print(n1.toString())
n1 = new Number(-1)
print(n1.toString())
print(n1.toString())
n1 = new Number(-1000000000)
print(n1.toString())
print(n1.toString())
n1 = new Number(-1000000000.1233444)
print(n1.toString())
print(n1.toString())
let n2 = new Number(10000.1234)
print(n2.toString())
print(n2.toString())
n2 = new Number(1000)
print(n2.toString())
print(n2.toString())
n2 = new Number(10000123456)
print(n2.toString())
print(n2.toString())
n2 = new Number(0.000000000010000123456)
print(n2.toString())
print(n2.toString())
n2 = new Number(0.000000000010000123456e10)
print(n2.toString())
print(n2.toString())
n2 = new Number(123456e10)
print(n2.toString())
print(n2.toString())

// math.atanh
try {
    const bigIntTest = -2147483647n;
    const test = Math.atanh(bigIntTest);
} catch(e) {
    print(e);
};

var s = (2.2250738585072e-308).toString(36)
print(s)

print(Number.parseInt("4294967294"))
print(Number.parseInt("2147483648"))

print(Number.parseFloat("10000000000000000000000.0"));


print("builtins number end");