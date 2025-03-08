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
 * @tc.name: isin
 * @tc.desc: test isin. Test whether the return value of IsIn is exception while input para is not a ECMA obj.
 * @tc.type: FUNC
 */
{
    try {
        1 in 0;
        let tmpArr = [0];
    } catch (e) {
        print(e);
    }
}

{
    const v5 = [-3];
    const a8 = new Uint8Array(2);
    v5.__proto__ = a8;
    print(-3 in v5);
}