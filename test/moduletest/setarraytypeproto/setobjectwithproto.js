/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
 * @tc.name: setarraytypeproto
 * @tc.desc: test SetPrototype while proto is an array.
 * @tc.type: FUNC
 * @tc.require: issue12457
 */

// While proto is an array, its hclass must be cloned before setting to an object as prototype.
{
    let regexp = new RegExp();
    let reg_proto = RegExp.prototype;
    let tmp;
    for (let i = 0; i < 100; i++) {
        tmp = i;
    }
    let arr = [1,2,3,4];
    let arr_1 = [5,6];
    let con = regexp.constructor;
    reg_proto.__proto__ = arr;
    reg_proto.__proto__ = arr_1;
    arr[undefined] = 0;
    let valof_ = reg_proto.valueOf();
    reg_proto.a = arr;
    // Before PR14079 was merged, this usecase will cause cpp crash. So there's no need to add assertion statement.
}

test_end();
