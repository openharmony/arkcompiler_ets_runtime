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
 * @tc.name:barrier
 * @tc.desc:test barrier
 * @tc.type: FUNC
 * @tc.require: issueIACEAI
 */

// @ts-nocheck
declare function print(str: any): string;

print("========== Test Define Sendable Function ==========")
function fooSendable()
{
    // define a sendable func
    'use sendable'
    print("fooSendable");
}

fooSendable()

print("========== Test Define Normal Function ==========")
function foo()
{
    // define a Normal func
    print("foo");
}

foo()

print("========== Test IC ==========")

function fun1(n) {
    function fun2(o) {
        return o.a;
    }

    let obj = {a: 1};
    for (let i = 0; i < n; i++) {
        fun2(obj);
    }
    return fun2(obj);
}

print(fun1(100));
print(fun1(1));


