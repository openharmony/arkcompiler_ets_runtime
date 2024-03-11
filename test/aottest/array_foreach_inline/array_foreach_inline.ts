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

declare function assert_equal(a: Object, b: Object):void;
let array1:numeber[] = [4, 5, 6];

const expectedArray1 = [4, 5, 6];
let i = 0;
array1.forEach((element) => {
    assert_equal(element, expectedArray1[i]);
    i++;
});

i = 0;
array1.forEach((element) => {
    array1[array1.length] = array1.length
    assert_equal(element, expectedArray1[i]);
    i++;
});

const expectedArray2 = [4, 6, 3, 4, 5];
i = 0;
array1.forEach((element) => {
    delete array1[1];
    assert_equal(element, expectedArray2[i]);
    i++;
});

assert_equal(array1.length, 6)

