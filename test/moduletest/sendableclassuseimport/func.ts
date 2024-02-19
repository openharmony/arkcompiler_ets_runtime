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
 * @tc.name:sendableclassuseimport
 * @tc.desc:test sendableclassuseimport
 * @tc.type: FUNC
 * @tc.require: issue#I91CQ6
 */
export class Test1 {
    value1:string;
    constructor(value1:string) {
        this.value1 = value1;
    }
    foo(): string {
        return "foo():" + this.value1;
    }
}

export function foo2(a1 : Test1, a2 : string) :string {
    return a1.foo() + a2;
}