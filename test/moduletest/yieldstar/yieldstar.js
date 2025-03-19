/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
 * @tc.name:yieldstar
 * @tc.desc:test yield *
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
function *foo1() {
    yield 1
    yield 2
}
  
function *foo2() {
    yield *foo1()
}

var p = foo2()
var a = p.next()
assert_equal(a.value,1);
assert_equal(a.done, false);
var b = p.next()
assert_equal(b.value, 2);
assert_equal(b.done, false);
var c = p.next()
assert_equal(c.value, undefined);
assert_equal(c.done, true);

test_end();