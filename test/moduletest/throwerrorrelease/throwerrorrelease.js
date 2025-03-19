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
 * @tc.name:throwerrorrelease
 * @tc.desc:test throwerror release
 * @tc.type: FUNC
 */
let v28 = "bigint";
const v29 = [v28, this];
v29[268435439] = this;
const v36 = `
this.minimumFractionDigits = 5.1000848328408265;
[a-zA-Z0-9]
`;
try {
    const v41 = v36.replaceAll(0, v29);
} catch (e) {
    print(e);
}