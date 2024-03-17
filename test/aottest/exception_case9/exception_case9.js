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

var testTxt = ["exception_case9.js:20:20", "exception_case9.js:24:24", "exception_case9.js:28:1"];
var i = 0;
try {
    function foo() {
        a.b
    }
    let o = {
        get name() {
            foo()
        },
        set name(a) {}
    }
    o.name;
    assert_unreachable();
} catch (e) {
    assert_equal(e.message, "a is not defined");
    let stack = e.stack
    let array = stack.split('\n')
    for (let line of array) {
        let start = line.lastIndexOf('/') + 1
        let end = line.length - 1
        if (start < end) {
            assert_equal(line.slice(start, end), testTxt[i]);
            i++;
        } else {
            print(line)
        }
    }
}