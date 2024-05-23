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

print("builtins number parseint");
print(Number.parseInt())
print(Number.parseInt(undefined))
print(Number.parseInt(null))
print(Number.parseInt(123))
print(Number.parseInt(new Number(-2)))
print(Number.parseInt("10.123"))
print(Number.parseInt("-0"))
print(Number.parseInt("0"))
print(Number.parseInt("-1000, 0"))
print(Number.parseInt("-1000"))
print(Number.parseInt("  01 ", "1"))
print(Number.parseInt("123456e10"))
print(Number.parseInt("1.0"))
print(Number.parseInt("1e0"))
print(Number.parseInt("10.0e-0"))
print(Number.parseInt("10.0e0"))
print(Number.parseInt("100.0E1"))
print(Number.parseInt("100", 2))
print(Number.parseInt("100", 4))
print(Number.parseInt("100", 8))
print(Number.parseInt("100", 10))
print(Number.parseInt("100", 16))

print("builtins global parseint");
print(parseInt())
print(parseInt(undefined))
print(parseInt(null))
print(parseInt(123))
print(parseInt(new Number(-2)))
print(parseInt("10.123"))
print(parseInt("-0"))
print(parseInt("0"))
print(parseInt("-1000, 0"))
print(parseInt("-1000"))
print(parseInt("  01 ", "1"))
print(parseInt("123456e10"))
print(parseInt("1.0"))
print(parseInt("1e0"))
print(parseInt("10.0e-0"))
print(parseInt("10.0e0"))
print(parseInt("100.0E1"))
print(parseInt("100", 2))
print(parseInt("100", 4))
print(parseInt("100", 8))
print(parseInt("100", 10))
print(parseInt("100", 16))

print("builtins number isnan");
print(Number.isNaN());
print(Number.isNaN(undefined));
print(Number.isNaN(null));
print(Number.isNaN(NaN));
print(Number.isNaN(0));
print(Number.isNaN(1));
print(Number.isNaN(-123));
print(Number.isNaN(1.23));
print(Number.isNaN(-1.23e10));
print(Number.isNaN(NaN, 1));
print(Number.isNaN(NaN + 1));
print(Number.isNaN(0/0));
print(Number.isNaN(Infinity));

print("builtins global isnan");
print(isNaN());
print(isNaN(undefined));
print(isNaN(null));
print(isNaN(NaN));
print(isNaN(0));
print(isNaN(1));
print(isNaN(-123));
print(isNaN(1.23));
print(isNaN(-1.23e10));
print(isNaN(NaN, 1));
print(isNaN(NaN + 1));
print(isNaN(0/0));
print(isNaN(Infinity));

print("builtins number isfinite");
print(Number.isFinite());
print(Number.isFinite(undefined));
print(Number.isFinite(null));
print(Number.isFinite(NaN));
print(Number.isFinite(Infinity));
print(Number.isFinite(-Infinity));
print(Number.isFinite(0.0));
print(Number.isFinite(-0.0));
print(Number.isFinite(123));
print(Number.isFinite(1.23));

print("builtins global isfinite");
print(isFinite());
print(isFinite(undefined));
print(isFinite(null));
print(isFinite(NaN));
print(isFinite(Infinity));
print(isFinite(-Infinity));
print(isFinite(0.0));
print(isFinite(-0.0));
print(isFinite(123));
print(isFinite(1.23));

print("builtins number isinteger");
print(Number.isInteger());
print(Number.isInteger(undefined));
print(Number.isInteger(null));
print(Number.isInteger(NaN));
print(Number.isInteger(Infinity));
print(Number.isInteger(0));
print(Number.isInteger(-0));
print(Number.isInteger(-0.0));
print(Number.isInteger(123));
print(Number.isInteger(-123));
print(Number.isInteger(1.23));
print(Number.isInteger(-1.23));
print(Number.isInteger("2"));
print(Number.isInteger(9007199254740991));
print(Number.isInteger(9007199254740991.0));
print(Number.isInteger(9007199254740991.1));
print(Number.isInteger(9007199254740991.5));
print(Number.isInteger(9007199254740992.0));
print(Number.isInteger(-9007199254740991));
print(Number.isInteger(-9007199254740991.0));
print(Number.isInteger(-9007199254740991.1));
print(Number.isInteger(-9007199254740991.5));
print(Number.isInteger(-9007199254740992.0));
print(Number.isInteger(1.0e22));
print(Number.isInteger(-1.0e22));

print("builtins number issafeinteger");
print(Number.isSafeInteger());
print(Number.isSafeInteger(undefined));
print(Number.isSafeInteger(null));
print(Number.isSafeInteger(NaN));
print(Number.isSafeInteger(Infinity));
print(Number.isSafeInteger(0));
print(Number.isSafeInteger(-0));
print(Number.isSafeInteger(-0.0));
print(Number.isSafeInteger(123));
print(Number.isSafeInteger(-123));
print(Number.isSafeInteger(1.23));
print(Number.isSafeInteger(-1.23));
print(Number.isSafeInteger("2"));
print(Number.isSafeInteger(9007199254740991));
print(Number.isSafeInteger(9007199254740991.0));
print(Number.isSafeInteger(9007199254740991.1));
print(Number.isSafeInteger(9007199254740991.5));
print(Number.isSafeInteger(9007199254740992.0));
print(Number.isSafeInteger(-9007199254740991));
print(Number.isSafeInteger(-9007199254740991.0));
print(Number.isSafeInteger(-9007199254740991.1));
print(Number.isSafeInteger(-9007199254740991.5));
print(Number.isSafeInteger(-9007199254740992.0));
print(Number.isSafeInteger(1.0e22));
print(Number.isSafeInteger(-1.0e22));

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