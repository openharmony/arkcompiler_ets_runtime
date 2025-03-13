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

let res1 = true;
for (let i = 0; i < 20; i++) {
    var sample1 = new Uint8ClampedArray([0]);
    var result1 = sample1.map(function() {
        return 123.622223;
    });
    if (result1 != 124) {
        res1 = false;
    }
}
assert_equal(res1, true);

let res2 = true;
for (let i = 0; i < 20; i++) {
    var sample2 = new Uint8ClampedArray([0]);
    var result2 = sample2.map(function() {
        return 32768;
    });
    if (result2 != 255) {
        res2 = false;
    }
}
assert_equal(res2, true);

let res3 = true;
for (let i = 0; i < 20; i++) {
    var sample3 = new Uint8ClampedArray([0]);
    var result3 = sample3.map(function() {
        return -32769;
    });
    if (result3 != 0) {
        res3 = false;
    }
}
assert_equal(res3, true);

{
    let arr1 = new Uint8Array(512);
    let i = 0;
    while (i < 128) {
        i++;
    }
    let arr2 = new Uint16Array(930);
    function myFunc() {
        let tmp = arr2[0];
    }
    arr2.__proto__ = arr1;
    let newArr = arr2.map(myFunc);
    assert_equal(newArr instanceof Uint8Array, true);
}

test_end();
