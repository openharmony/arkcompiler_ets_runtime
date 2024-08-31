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
 * @tc.name:decodeuricomponent
 * @tc.desc:test decodeuricomponent
 * @tc.type: FUNC
 * @tc.require: issueI7CTF7
 */
let uri="%c2%aa%66%55%58%c2%83%c2%93%00%c2%89%c3%96%08%58%c2%b4%c3%bd%46";
let uri_encode=decodeURIComponent(uri);
print(encodeURIComponent(uri_encode));

let uri1="%00";
let uri_encode1=decodeURIComponent(uri1);
print(encodeURIComponent(uri_encode1));

try {
    let invalidURI = "%c0%80";
    decodeURIComponent(invalidURI);
} catch(err) {
    print(err.name);
}

try {
    let invalidURI1 = "http://example.com/?q=%";
    decodeURI(invalidURI1);
} catch(err) {
    print(err);
}

try {
    let invalidURI2 = "%E4%25BD%2593%25E8%2582%25B2";
    decodeURIComponent(invalidURI2);
} catch(err) {
    print(err);
}

let header = "https://ifs.tanx.com/display_ifs?i~";
try {
    let invalidURI = header + "%F5%8F%BF%BF";
    decodeURIComponent(invalidURI);
} catch (err) {
    print(err.name);
}

try {
    let invalidURI = header + "%F4%90%BF%BF";
    decodeURIComponent(invalidURI);
} catch (err) {
    print(err.name);
}

{
    let validURI = header + "%F4%8F%BF%BF";
    decodeURIComponent(validURI);
    print("decode successful");
}

var uri0="https://www.runoob.com/my t" + "est.php?name=ståle&car=saab";
var uri11 = uri0 + "/jfdlskafasfd";
var uri2 = uri0 + "/2389018203";
var uri3 = uri11 + "/jd2931dsafdsa";
var uri4 = uri3 + "/jd2931wjeiojfwre";
print(encodeURIComponent(uri0));
print(encodeURIComponent(uri11));
print(encodeURIComponent(uri2));
print(encodeURIComponent(uri3));
print(encodeURIComponent(uri4));

print(decodeURIComponent(uri0));
print(decodeURIComponent(uri11));
print(decodeURIComponent(uri2));
print(decodeURIComponent(uri3));
print(decodeURIComponent(uri4));

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
    print(result)
}

let invalid_uri_list = ["%ED%A0%80", "%ED%AF%BF", "%ED%B0%80", "%ED%BF%BF", "%E0%9F%BF"]
for (let i = 0; i < invalid_uri_list.length; ++i) {
    try {
        decodeURIComponent(invalid_uri_list[i]);
    } catch (err) {
        print(err.name)
    }
}