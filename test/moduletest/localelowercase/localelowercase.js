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
assert_equal(en.toLocaleLowerCase(),'english');
assert_equal(en.toLocaleLowerCase('zh-CN'),'english');
assert_equal(en.toLocaleLowerCase('en-US'),'english');

const tr = 'İstanbul';
assert_equal(tr.toLocaleLowerCase(),'i̇stanbul');
assert_equal(tr.toLocaleLowerCase('zh-CN'),'i̇stanbul');
assert_equal(tr.toLocaleLowerCase('en-US'),'i̇stanbul');

let el = 'Αυτό είΝαι ένα δοκιμαστΙκό κείμενο.';
assert_equal(el.toLocaleLowerCase(),'αυτό είναι ένα δοκιμαστικό κείμενο.');
assert_equal(el.toLocaleLowerCase('zh-CN'),'αυτό είναι ένα δοκιμαστικό κείμενο.');
assert_equal(el.toLocaleLowerCase('en-US'),'αυτό είναι ένα δοκιμαστικό κείμενο.');
assert_equal(el.toLocaleLowerCase('el'),'αυτό είναι ένα δοκιμαστικό κείμενο.');

test_end();