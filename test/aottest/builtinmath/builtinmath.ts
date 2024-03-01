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

declare function print(arg:any):string;
declare function assert_true(condition: boolean):void;
declare function assert_equal(a: Object, b: Object):void;
function replace(a : number)
{
    return a;
}
let len:number = 1;
len = 1 / Math.sqrt(len);
assert_equal(len, 1);

len = 9 / len;
len = Math.sqrt(len);
assert_equal(len, 3);
print(NaN);
len = Math.sqrt(NaN);
assert_true(isNaN(len));

len = Math.sqrt(+0); // 0
assert_equal(len, 0);
len = Math.sqrt(Number.POSITIVE_INFINITY); // Infinity
assert_equal(len, Infinity);
len = Math.sqrt(Number.NEGATIVE_INFINITY); // NaN
assert_true(isNaN(len));
len = Math.sqrt(-1); // NaN
assert_true(isNaN(len));
function sqrt()
{
    Math.sqrt = replace;
    len = Math.sqrt(9)
    assert_equal(len, 9);
}
sqrt()

len = Math.cos(0); // 1
assert_equal(len, 1);
len = Math.cos(1); // 0.5....
assert_equal(len, 0.5403023058681398);
function cos()
{
    Math.cos = replace;
    len = Math.cos(1);
    assert_equal(len, 1);
}
cos()

len = Math.sin(0); // 0
assert_equal(len, 0);
len = Math.sin(1); // 0.84
assert_equal(len, 0.8414709848078965);
len = Math.sin(Math.PI / 2);
assert_equal(len, 1);
function sin()
{
    Math.sin = replace;
    len = Math.sin(1)
    assert_equal(len, 1);
}
sin()

len = Math.acos(0.5);// 1.0471975511965979
assert_equal(len, 1.0471975511965979);
function acos()
{
    Math.acos = replace
    len = Math.acos(0.5)
    assert_equal(len, 0.5);
}
acos()

len = Math.atan(2); // 1.1071487177940904
assert_equal(len, 1.1071487177940904);
function atan()
{
    Math.atan = replace
    len = Math.atan(2)
    assert_equal(len, 2);
}
atan()

len = Math.abs(Number.NaN);
assert_true(isNaN(len));
len = Math.abs(-Number.NaN);
assert_true(isNaN(len));
len = Math.abs(Number.NEGATIVE_INFINITY);
assert_equal(len, Infinity);
len = Math.abs(Number.POSITIVE_INFINITY);
assert_equal(len, Infinity);
len = Math.abs(9.6);
assert_equal(len, 9.6);
len = Math.abs(6);
assert_equal(len, 6);
len = Math.abs(-9.6);
assert_equal(len, 9.6);
len = Math.abs(-6);
assert_equal(len, 6);
function abs()
{
    Math.abs = replace
    len = Math.abs(-9.6)
    assert_equal(len, -9.6);
}
abs()

len = Math.floor(Number.NaN);
assert_true(isNaN(len));
len = Math.floor(-Number.NaN);
assert_true(isNaN(len));
len = Math.floor(Number.NEGATIVE_INFINITY);
assert_equal(len, -Infinity);
len = Math.floor(Number.POSITIVE_INFINITY);
assert_equal(len, Infinity);
len = Math.floor(9.6);
assert_equal(len, 9);
len = Math.floor(-9.6);
assert_equal(len, -10);
function floor()
{
    Math.floor = replace
    len = Math.floor(-9.6)
    assert_equal(len, -9.6);
}
floor()
