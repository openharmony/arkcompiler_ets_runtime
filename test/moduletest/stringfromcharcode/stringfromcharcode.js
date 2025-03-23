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
 * @tc.name:fromCharCode
 * @tc.desc:test String.fromCharCode and charat
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */

var str = String.fromCharCode(0);
var str1 = String.fromCharCode(56);
var str2 = String.fromCharCode(90);
var str3 = String.fromCharCode(113);
assert_equal(str1,"8");
assert_equal(str2,"Z");
assert_equal(str3,"q");
var obj = {};
obj[str1] = 'jjj1';
obj[str2] = 'jjj2';
obj[str3] = 'jjj3';
assert_equal(obj[8],"jjj1");
assert_equal(obj.Z,"jjj2");
assert_equal(obj.q,"jjj3");

var str4 = "wode每一天";
var str5 = "wodekk";
assert_equal(str4.charAt(4),"每");
assert_equal(str5.charAt(4),"k");
obj[str5.charAt(4)] = 'jjj4';
assert_equal(obj.k,"jjj4");


var str6 = "wojjj*432$@#$";
var str7 = "Π我的gljds&(%怕jfd"
assert_equal(str6.codePointAt(3),106);
assert_equal(str6.codePointAt(9),36);
assert_equal(str6.codePointAt(12),36);
assert_equal(str6.codePointAt(28),undefined);
assert_equal(str7.codePointAt(0),928);
assert_equal(str6.codePointAt(1),111);
assert_equal(str6.codePointAt(12),36);
assert_equal(str6.codePointAt(284),undefined);

var str8 = "meiyou";
var str9 = "haodeha";
var str10 = "wodeyisishi";
assert_equal(str8.concat(str9),"meiyouhaodeha");
assert_equal(str8.concat(str9, str10),"meiyouhaodehawodeyisishi");
assert_equal(str8.concat(str9, '  hh ', str10),"meiyouhaodeha  hh wodeyisishi");

var str11 = "djfaDJKLAD";
var str12 = "djfaDJKLADf大家发";
var str13 = "DJKLAD";

assert_equal(str11.toLowerCase(),"djfadjklad");
assert_equal(str12.toLowerCase(),"djfadjkladf大家发");
assert_equal(str13.toLowerCase(),"djklad");

test_end();