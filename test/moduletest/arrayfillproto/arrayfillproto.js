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


/*
 * @tc.name:array fill proto
 * @tc.desc:test array fill proto
 */
let arr1 = new Array(500);
arr1.__proto__.length = 10;
arr1.__proto__.fill(233, 0, 7);
let arr2 = new Array(500);
let arr3 = arr1.concat(arr2);
print(arr3[0], arr3[1], arr3[2], arr3[500], arr3[501], arr3[502]);