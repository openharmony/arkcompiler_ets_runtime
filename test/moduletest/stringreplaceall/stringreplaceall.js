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
 * @tc.name:stringreplaceall
 * @tc.desc:test stringreplaceall
 * @tc.type: FUNC
 * @tc.require: issueIATECS
 */

// case1 - string: utf8, search: not regexp, replace: no dollar
let str1 = "Hello, world";
let res1 = str1.replaceAll("o", "o-o");
assert_equal(res1,"Hello-o, wo-orld");

// case2 - string: utf8, search: regexp, replace: no dollar
let res2 = str1.replaceAll(/o/g, "o-o");
assert_equal(res2,"Hello-o, wo-orld");

// case3 - string: utf8, search: regexp, replace: dollar
let res3 = str1.replaceAll(/o/g, "$&-$&");
assert_equal(res3,"Hello-o, wo-orld");

// case4 - string: utf8, search: not regexp, replace: dollar
let res4 = str1.replaceAll("o", "$&-$&");
assert_equal(res4,"Hello-o, wo-orld");

// case5 - string: utf8, search: not regexp, replace: function
let res5 = str1.replaceAll("o", String);
assert_equal(res5,"Hello, world");

// case6 - string: utf16, search: not regexp, replace: no dollar
let str3 = "你好，世界！你好，世界！";
let res6 = str3.replaceAll("好", "好-好");
assert_equal(res6,"你好-好，世界！你好-好，世界！");

// case7 - string: utf16, search: regexp, replace: no dollar
let res7 = str3.replaceAll(/好/g, "好-好");
assert_equal(res7,"你好-好，世界！你好-好，世界！");

// case8 - string: utf16, search: regexp, replace: dollar
let res8 = str3.replaceAll(/好/g, "$&-$&");
assert_equal(res8,"你好-好，世界！你好-好，世界！");

// case9 - string: utf16, search: not regexp, replace: dollar
let res9 = str3.replaceAll("好", "$&-$&");
assert_equal(res9,"你好-好，世界！你好-好，世界！");

// case10 - string: utf16, search: not regexp, replace: function
let res10 = str3.replaceAll("好", String);
assert_equal(res10,"你好，世界！你好，世界！");

test_end();