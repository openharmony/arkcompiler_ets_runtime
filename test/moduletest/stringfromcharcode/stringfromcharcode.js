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
print(str1);
print(str2);
print(str3);
var obj = {};
obj[str1] = 'jjj1';
obj[str2] = 'jjj2';
obj[str3] = 'jjj3';
print(obj[8]);
print(obj.Z);
print(obj.q);

var str4 = "wode每一天";
var str5 = "wodekk";
print(str4.charAt(4));
print(str5.charAt(4));
obj[str5.charAt(4)] = 'jjj4';
print(obj.k);


var str6 = "wojjj*432$@#$";
var str7 = "Π我的gljds&(%怕jfd"
print(str6.codePointAt(3));
print(str6.codePointAt(9));
print(str6.codePointAt(12));
print(str6.codePointAt(28));
print(str7.codePointAt(0));
print(str6.codePointAt(1));
print(str6.codePointAt(12));
print(str6.codePointAt(284));

var str8 = "meiyou";
var str9 = "haodeha";
var str10 = "wodeyisishi";
print(str8.concat(str9));
print(str8.concat(str9, str10));
print(str8.concat(str9, '  hh ', str10));

var str11 = "djfaDJKLAD";
var str12 = "djfaDJKLADf大家发";
var str13 = "DJKLAD";

print(str11.toLowerCase());
print(str12.toLowerCase());
print(str13.toLowerCase());