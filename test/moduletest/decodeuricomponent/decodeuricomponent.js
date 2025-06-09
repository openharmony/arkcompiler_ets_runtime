/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 * @tc.name:decodeuricomponent
 * @tc.desc:test decodeuricomponent
 * @tc.type: FUNC
 * @tc.require: issueI7CTF7
 */
let uri="%c2%aa%66%55%58%c2%83%c2%93%00%c2%89%c3%96%08%58%c2%b4%c3%bd%46";
let uri_encode=decodeURIComponent(uri);
assert_equal(encodeURIComponent(uri_encode), "%C2%AAfUX%C2%83%C2%93%00%C2%89%C3%96%08X%C2%B4%C3%BDF");
uri_encode=decodeURI(uri);
assert_equal(encodeURI(uri_encode), "%C2%AAfUX%C2%83%C2%93%00%C2%89%C3%96%08X%C2%B4%C3%BDF");

let uri1="%00";
let uri_encode1=decodeURIComponent(uri1);
assert_equal(encodeURIComponent(uri_encode1), "%00");
uri_encode1=decodeURI(uri1);
assert_equal(encodeURI(uri_encode1), "%00");

let errorStr = "";
try {
    let invalidURI = "%c0%80";
    decodeURIComponent(invalidURI);
} catch(err) {
    errorStr = err.toString();
}
assert_equal(errorStr, "URIError: DecodeURI: invalid character: %c0%80");

try {
    let invalidURI1 = "http://example.com/?q=%";
    decodeURI(invalidURI1);
} catch(err) {
    errorStr = err.toString();
}
assert_equal(errorStr, "URIError: DecodeURI: invalid character: http://example.com/?q=%");

try {
    let invalidURI2 = "%E4%25BD%2593%25E8%2582%25B2";
    decodeURIComponent(invalidURI2);
} catch(err) {
    errorStr = err.toString();
}
assert_equal(errorStr, "URIError: DecodeURI: invalid character: %E4%25BD%2593%25E8%2582%25B2");

let header = "https://ifs.tanx.com/display_ifs?i~";
try {
    let invalidURI = header + "%F5%8F%BF%BF";
    decodeURIComponent(invalidURI);
} catch(err) {
    errorStr = err.toString();
}
assert_equal(errorStr, "URIError: DecodeURI: invalid character: https://ifs.tanx.com/display_ifs?i~%F5%8F%BF%BF");

try {
    let invalidURI = header + "%F4%90%BF%BF";
    decodeURIComponent(invalidURI);
} catch(err) {
    errorStr = err.toString();
}
assert_equal(errorStr, "URIError: DecodeURI: invalid character: https://ifs.tanx.com/display_ifs?i~%F4%90%BF%BF");

{
    let validURI = header + "%F4%8F%BF%BF";
    decodeURIComponent(validURI);
}

var uri0="https://www.runoob.com/my t" + "est.php?name=ståle&car=saab";
var uri11 = uri0 + "/jfdlskafasfd";
var uri2 = uri0 + "/2389018203";
var uri3 = uri11 + "/jd2931dsafdsa";
var uri4 = uri3 + "/jd2931wjeiojfwre";
assert_equal(encodeURIComponent(uri0),
    "https%3A%2F%2Fwww.runoob.com%2Fmy%20test.php%3Fname%3Dst%C3%A5le%26car%3Dsaab");
assert_equal(encodeURIComponent(uri11),
    "https%3A%2F%2Fwww.runoob.com%2Fmy%20test.php%3Fname%3Dst%C3%A5le%26car%3Dsaab%2Fjfdlskafasfd");
assert_equal(encodeURIComponent(uri2),
    "https%3A%2F%2Fwww.runoob.com%2Fmy%20test.php%3Fname%3Dst%C3%A5le%26car%3Dsaab%2F2389018203");
assert_equal(encodeURIComponent(uri3),
    "https%3A%2F%2Fwww.runoob.com%2Fmy%20test.php%3Fname%3Dst%C3%A5le%26car%3Dsaab%2Fjfdlskafasfd%2Fjd2931dsafdsa");
assert_equal(encodeURIComponent(uri4),
    "https%3A%2F%2Fwww.runoob.com%2Fmy%20test.php%3Fname%3Dst%C3%A5le%26car%3Dsaab%2Fjfdlskafasfd%2Fjd2931dsafdsa%2Fj" +
    "d2931wjeiojfwre");

assert_equal(encodeURI(uri0),
    "https://www.runoob.com/my%20test.php?name=st%C3%A5le&car=saab");
assert_equal(encodeURI(uri11),
    "https://www.runoob.com/my%20test.php?name=st%C3%A5le&car=saab/jfdlskafasfd");
assert_equal(encodeURI(uri2),
    "https://www.runoob.com/my%20test.php?name=st%C3%A5le&car=saab/2389018203");
assert_equal(encodeURI(uri3),
    "https://www.runoob.com/my%20test.php?name=st%C3%A5le&car=saab/jfdlskafasfd/jd2931dsafdsa");
assert_equal(encodeURI(uri4),
    "https://www.runoob.com/my%20test.php?name=st%C3%A5le&car=saab/jfdlskafasfd/jd2931dsafdsa/jd2931wjeiojfwre");

assert_equal(decodeURIComponent(uri0),
    "https://www.runoob.com/my test.php?name=ståle&car=saab");
assert_equal(decodeURIComponent(uri11),
    "https://www.runoob.com/my test.php?name=ståle&car=saab/jfdlskafasfd");
assert_equal(decodeURIComponent(uri2),
    "https://www.runoob.com/my test.php?name=ståle&car=saab/2389018203");
assert_equal(decodeURIComponent(uri3),
    "https://www.runoob.com/my test.php?name=ståle&car=saab/jfdlskafasfd/jd2931dsafdsa");
assert_equal(decodeURIComponent(uri4),
    "https://www.runoob.com/my test.php?name=ståle&car=saab/jfdlskafasfd/jd2931dsafdsa/jd2931wjeiojfwre");

{
    var result = true;
    var arr = [
    [0x00, 0x2F],
    [0x47, 0x60],
    ];
    for (var i = 0; i < arr.length; i++) {
        for (var j = arr[i][0]; j <= arr[i][1]; j++) {
            try {
                decodeURIComponent("%" + String.fromCharCode(j)+ "1");
                decodeURIComponent("%" + "1" + String.fromCharCode(j));
                decodeURIComponent("%C0%" + String.fromCharCode(j, j));
                decodeURIComponent("%E0%" + String.fromCharCode(j, j) + "%A0");
                decodeURIComponent("%E0" + "%A0%" + String.fromCharCode(j, j));
                decodeURIComponent("%F0%" + String.fromCharCode(j, j) + "%A0%A0");
                decodeURIComponent("%F0" + "%A0%" + String.fromCharCode(j, j) + "%A0");
                decodeURIComponent("%F0" + "%A0%A0%" + String.fromCharCode(j, j));
                result = false;
            } catch (e) {
                if ((e instanceof URIError) !== true) {
                    result = false;
                }
            }
        }
    }
    assert_true(result);
}

let invalid_uri_list = ["%ED%A0%80", "%ED%AF%BF", "%ED%B0%80", "%ED%BF%BF", "%E0%9F%BF"]
for (let i = 0; i < invalid_uri_list.length; ++i) {
    try {
        decodeURIComponent(invalid_uri_list[i]);
    } catch (err) {
        errorStr = err.toString();
    }
    assert_equal(errorStr, "URIError: DecodeURI: invalid character: " + invalid_uri_list[i]);
}

let URIStr0 = "你好HelloمرحباأهلاओइПриветनमस्तेΓεια σαςこんにちは안녕하세요សួស្តី"
assert_equal(encodeURI(URIStr0),
    "%E4%BD%A0%E5%A5%BDHello%D9%85%D8%B1%D8%AD%D8%A8%D8%A7%D8%A3%D9%87%D9%84%D8%A7%E0%A4%93%E0%A4%87%D0%9F%D1%80%D0%B" +
    "8%D0%B2%D0%B5%D1%82%E0%A4%A8%E0%A4%AE%E0%A4%B8%E0%A5%8D%E0%A4%A4%E0%A5%87%CE%93%CE%B5%CE%B9%CE%B1%20%CF%83%CE%B1" +
    "%CF%82%E3%81%93%E3%82%93%E3%81%AB%E3%81%A1%E3%81%AF%EC%95%88%EB%85%95%ED%95%98%EC%84%B8%EC%9A%94%E1%9E%9F%E1%9E%" +
    "BD%E1%9E%9F%E1%9F%92%E1%9E%8F%E1%9E%B8");

test_end();