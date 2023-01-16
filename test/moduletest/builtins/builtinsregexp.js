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

/*
 * @tc.name:builtins
 * @tc.desc:test builtins regexp
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
print("builtins regexp start");

// Test1 - Regexp backward
const url = 'https://designcloud.uiplus.huawei.com/tool//materialServer/upload/images/20210608_5V0J5lVh4xVNYx0AUE.jpg';
const data = url.match(/(?<=\/)\w+(.jpg)$/);
print(data);

print("builtins regexp end");