/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

function GetFormat(character: stirng): string {
    switch (character) {
        case 'a':
            return 'ONE';
        case 'b':
            return 'TWO';
        case 'c':
            return 'THREE';
        case 'd':
            return 'FOUR';
        case 'e':
            return 'FIVE';
        case 'f':
            return 'SIX';
        case 'g':
            return 'SEVEN';
        case 'h':
            return 'EIGHT';
        case 'i':
            return 'NINE';
        case 'j':
            return 'TEN';
        default:
            return character;
    }
}

const stringArray: string[] = ['a', 'c', 'e', 'g', 'i', 'k', 'l', 'm', 'n', 'o', 'one', 'two', 'three'];

function Test(array) {
    for (let ch of array) {
        GetFormat(ch);
    }
}

Test(stringArray);

if (ArkTools.isAOTCompiled(GetFormat)) {
    Test([0]);
    Test([GetFormat]);
    var tmp = new String('123');
    Test([tmp.concat('456')]);
} else {
    print(false);
}
