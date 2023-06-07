/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
 * @tc.name:regexp
 * @tc.desc:test Regexp
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
var reg = /[\x5d-\x7e]/i;
var result = reg.test("a");
print(result);
// true

var reg1 = new RegExp("^[-+]?([0-9]+)?(\\٫[0-9]{1,})?$");
var result1 = reg1.test('0٫0000000000001');
print(result1);
// true

var reg2 = /^[Α-ώ]+$/i
print(reg2.test('άέήίΰϊϋόύώ'));
// true
print(reg2.test('ΆΈΉΊΪΫΎΏ'));
// true
print(reg2.test('αβγδεζηθικλμνξοπρςστυφχψω'));
// true
print(reg2.test('ΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩ'));
// true

let reg3 =/^[A-Z0-9_\-]*$/i
print(reg3.test(''))
//true