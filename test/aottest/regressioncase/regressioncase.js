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

// case 1
try {
	var base = null;
	var prop = {
		toString: function () {
			print("toString");
			throw "111";
		},
	};
	base[prop];
} catch (e) {
	print("error");
}

// case 2
print(Number.NaN != Number.NaN);

// case 3
function _test() {
	this.x = 0.1;
}
var a = new _test();
print(a.x);

// case 4: n mod d = r
// If r = 0 and n < -0, return -0.
print(1 / (-1 % 1));
print(1 / (-1 % -1));
print(1 / (-3 % 1));
print(1 / (-3 % -1));
print(1 / (-3 % 3));
print(1 / (-3 % -3));
print(1 / (-3.3 % 3.3));
print(1 / (-3.3 % -3.3));

// case 5: mod
var a = {};
a._toString = function (value) {
	if (value === 0 && 1 / value === -Infinity) {
		return "-0";
	}
	return String(value);
};
var x;
x = -1;
print(a._toString((x %= -1)));

// case6: prototype
var a = [0];
a.length = 3;
Object.prototype[2] = 2;
print(a[2]);
