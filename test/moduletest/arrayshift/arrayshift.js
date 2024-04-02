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

var a1 = new Array(3);
for (let i = 0; i < 3; i++) {
    print(i in a1);
}
a1.shift()
for (let i = 0; i < 3; i++) {
    print(i in a1);
}

var arr1 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
var arr2 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];

var result1 = arr1.shift();
print(result1);
arr2.constructor = function () {
    return 88;
}
var result2 = arr2.shift();
print(result2);
