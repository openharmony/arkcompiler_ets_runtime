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
 * @tc.name:require
 * @tc.desc:test require
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
var b = require('./B.js');
var c = require('./C.js');
var index = require('.');
var strB = JSON.stringify(b);
var strC = JSON.stringify(c);
var strIndex = JSON.stringify(index);
assert_equal(strB + " and " + strC, '{"b":"objB_mes"} and {"c":"objC_mes"}');
assert_equal(strIndex, '{"index":"index loading..."}');

test_end();