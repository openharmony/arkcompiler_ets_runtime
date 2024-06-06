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
 * @tc.name:stringSlice
 * @tc.desc:test String.slice
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */

function testLocaleCompare() {
    const testCases = [
        // LineOrConstant v.s. LineOrConstant
        { str1: 'apple', str2: 'apple' },
        { str1: 'apple', str2: 'banana' },
        { str1: 'banana', str2: 'apple' },
        { str1: 'app', str2: 'apple' },
        { str1: 'application', str2: 'apple' },
        { str1: 'Apple', str2: 'apple' },
        { str1: '123', str2: '1234' },
        { str1: '1234', str2: '123' },
        { str1: 'Zebra', str2: 'apple' },
        { str1: 'appleappleappleappleappleapple', str2: 'appleappleappleappleappleapple' },
        { str1: 'appleappleappleappleappleapple', str2: 'appleappleapplebppleappleapple' },
        { str1: 'appleappleappleAppleappleapple', str2: 'appleappleappleappleappleapple' },
        // SliceString v.s. SliceString
        { str1: 'appleappleapple'.substring(0, 5), str2: 'appleappleapple'.substring(5, 10) },
        { str1: 'appleappleapple'.substring(1, 6), str2: 'appleappleapple'.substring(6, 11) },
        { str1: 'appleappleapple'.substring(0, 5), str2: 'appleappleapple'.substring(10, 15) },
        { str1: 'appleappleapple'.substring(0, 5), str2: 'appleappleapple'.substring(5, 11) },
        // LineOrConstant v.s. TreeString
        { str1: 'HelloWorld', str2: 'Hello'.concat('World') },
        { str1: 'Ni'.concat('Hao', 'Ya'), str2: 'NiHaoYa' },
        { str1: 'HelloWorld', str2: 'Hello'.concat('World ') },
        { str1: 'Ni'.concat('Hao', 'Ya', '1'), str2: 'NiHaoYa' },
        // TreeString v.s. TreeString
        { str1: 'Hell'.concat('oWorld'), str2: 'Hello'.concat('World') },
        { str1: 'Ni'.concat('Hao', 'Ya'), str2: 'NiHao'.concat('Ya') },
        { str1: 'Hello'.concat('a'), str2: 'Hello'.concat('b') },
        { str1: 'Ni'.concat('Hao', '8'), str2: 'NiHao'.concat('7') },
        // TreeString v.s. SliceString
        { str1: 'appleappleapple'.substring(5, 10), str2: 'app'.concat('le') },
        { str1: 'hello'.concat('world'), str2: 'ohelloworld'.substring(1) },
        { str1: 'hello'.concat('world'), str2: 'ohelloworld'.substring(2) },
        { str1: 'appleappleapple'.substring(5, 11), str2: 'app'.concat('le') },
        // LineOrConstant v.s. SliceString
        { str1: 'apple', str2: 'appleappleapple'.substring(5, 10) },
        { str1: 'appleappleapple'.substring(0, 5), str2: 'apple' },
        // Complex string compare
        { str1: 'hello'.concat('world').substring(2, 4), str2: 'hello'.substring(2, 3).concat('l') },
    ];
    testCases.forEach(testCase => {
        // default locale is en-US
        const result = testCase.str1.localeCompare(testCase.str2);
        print(result);
    });
    const testCasesUni = [
        // UTF8 v.s. UTF16
        { str1: 'café', str2: 'cafe' },
        { str1: 'café', str2: 'café' },
        { str1: 'resumé', str2: 'resume' },
        { str1: 'resume', str2: 'resumé' },
        { str1: ' naïve', str2: 'naïve' },
        { str1: 'naïve', str2: ' naïve' },
        { str1: 'élan', str2: 'elan' },
        { str1: 'élan', str2: 'e' + String.fromCharCode(0x0301) + 'lan' }
    ];
    testCasesUni.forEach(testCase => {
        const result = testCase.str1.localeCompare(testCase.str2, 'en-US');
        print(result);
    });
  }
  testLocaleCompare();

