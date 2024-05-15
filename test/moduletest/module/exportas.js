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
 * @tc.name:module
 * @tc.desc:test module
 * @tc.type: FUNC
 * @tc.require: issueI5RC2C
 */

var apple = "apple"
var str = "get fun"
var zip = "zip"
export {str, apple, zip}
export {str as tmp}
export function f() {
    print("load function f()");
}

const add = (a, b) => {
    print('add');
    return a + b;
};
export { add };
