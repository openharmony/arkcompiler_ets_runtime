/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
