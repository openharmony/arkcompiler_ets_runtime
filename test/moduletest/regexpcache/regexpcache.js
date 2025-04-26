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

const str = "SheSellsSeashellsByTheSeashoreTheShellsSheSellsAreSurelySeashells";
const regex = /She/;

// 1. RegExp.prototype.test()
let test_res1 = regex.test(str);
let test_res2 = regex.test(str);
assert_equal(test_res1, test_res2);

// 2. RegExp.prototype.exec()
let exec_res1 = regex.exec(str);
let exec_res2 = regex.exec(str);
assert_equal(exec_res1, exec_res2);

// 3. String.prototype.match()
const match_res1 = str.match(regex);
const match_res2 = str.match(regex);
assert_equal(match_res1, match_res2);

// 4. String.prototype.replace()
const replace_res1 = str.replace(regex, "He");
const replace_res2 = str.replace(regex, "He");
assert_equal(replace_res1, replace_res2);

// 5. String.prototype.search()
const search_res1 = str.search(regex);
const search_res2 = str.search(regex);
assert_equal(search_res1, search_res2);

// 6. String.prototype.split()
const split_res1 = str.split(regex);
const split_res2 = str.split(regex);
assert_equal(split_res1, split_res2);

test_end();
