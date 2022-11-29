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
 * @tc.name: localelowercase
 * @tc.desc: test toLocaleLowerCase
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
const en = "ENGLISH";
print(en.toLocaleLowerCase());
print(en.toLocaleLowerCase('zh-CN'));
print(en.toLocaleLowerCase('en-US'));

const tr = 'İstanbul';
print(tr.toLocaleLowerCase());
print(tr.toLocaleLowerCase('zh-CN'));
print(tr.toLocaleLowerCase('en-US'));

let el = 'Αυτό είΝαι ένα δοκιμαστΙκό κείμενο.';
print(el.toLocaleLowerCase());
print(el.toLocaleLowerCase('zh-CN'));
print(el.toLocaleLowerCase('en-US'));
print(el.toLocaleLowerCase('el'));