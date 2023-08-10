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
 * @tc.name:dataview
 * @tc.desc:test dataview
 * @tc.type: FUNC
 * @tc.require: issue#I7NUZM
 */
const buffer = new ArrayBuffer(16);
const view = new DataView(buffer);
view.setInt32({}, 0x1337, {});
print(view.getInt32({}, {}));