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
var holey_array = [1, 2, 3, 4, 5,,,,,,];
print(holey_array.slice(6, 7)[0]);
print(holey_array.slice(2, 3)[0]);

(function() {
    var array = [,];
	function slice() {
		return array.slice();
	}
	array.__proto__.push(5);
	var narr = slice();
	print(Object.getOwnPropertyDescriptor(narr, 0));
})();



// This use case takes a long time, but the corresponding bug is only repeated in the debug
let err = [];
err.length=100;
let err_len = 0;
function runNearStackLimit(f) {
    function t() {
        try {
            t();
        } catch (e) {
            err[err_len++]=e;
            f();
        }
    }; try {
        t();
    } catch (e) { }
}
const v7 = new Proxy(String, {});
function f0() {
    v7.bind();
}
runNearStackLimit(f0);
print("runNearStackLimit test success!");