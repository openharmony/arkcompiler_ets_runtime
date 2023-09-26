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

/*
 * @tc.name:createobjectwithbuffer
 * @tc.desc:test createobjectwithbuffer
 * @tc.type: FUNC
 * @tc.require: issueI84U4E
 */

function logit(a) {
    print(a.x)
    print(a.y)
    print(a[0])
    print(a[1])
}

function foo() {
    for (let i = 0; i < 1; ++i) {
        let a = {
            x : 1,
            y : "tik",
            z() { return 666 },
            0 : 0,
            1 : 1,
        }
        logit(a)
    }
}

function goo() {
    let a = {
        x : 1,
        y : "yyy",
        0 : 0,
        1 : 1,
    }
    logit(a)
}

function hoo() {
    for (let i = 0; i < 1; ++i) {
        let a = {
            444444444444 : 16,
        }
        print(a[444444444444]);
    }
}

foo();
goo();
hoo();
