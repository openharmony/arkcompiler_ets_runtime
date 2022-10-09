/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

var e1 = -10

function A() {
    var a0 = 2
    var a1 = 3
    var a2 = 4
    function B() {
        var b0 = 12
        var b1 = 13
        var b2 = 14
        function C() {
            print("patch: " + e1)
            print("patch: " + a0)
            print("patch: " + a1)
            print("patch: " + a2)
            print("patch: " + b0)
            print("patch: " + b1)
            print("patch: " + b2)
        }
        C()
    }
    B()
}