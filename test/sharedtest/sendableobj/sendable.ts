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

/*
 * @tc.name:sharedarray
 * @tc.desc:test sharedarray
 * @tc.type: FUNC
 * @tc.require: issueI8QUU0
 */

// @ts-nocheck
declare function print(str: any): string;
declare function isSendable(obj: lang.ISendable | Object): boolean;

class SendableClass {
    constructor() {
        "use sendable"
    }
}

class UnSendableClass {
    constructor() {
        "not sendable"
    }
}

sendCs = new SendableClass();
if (isSendable(sendCs)) {
    print("Sendable class is sendable");
} else {
    print("Sendable class is not sendable");
}

noSendCs = new UnSendableClass();
if (isSendable(noSendCs)) {
    print("UnSendable class is sendable");
} else {
    print("UnSendable class is not sendable");
}

let bool = true;
if (isSendable(bool)) {
    print("boolean is sendable");
} else {
    print("boolean is not sendable");
}


let str = "hello world";
if (isSendable(str)) {
    print("string is sendable");
} else {
    print("string is not sendable");
}

let num = 0;
if (isSendable(num)) {
    print("number is sendable");
} else {
    print("number is not sendable");
}

let bigInt = 124567890123456789012345678901234567890n;
if (isSendable(bigInt)) {
    print("bigInt is sendable");
} else {
    print("bigInt is not sendable");
}

function func()
{}
if (isSendable(func)) {
    print("function is sendable");
} else {
    print("function is not sendable");
}

function sendableFunc()
{
    "use sendable"
}
if (isSendable(func)) {
    print("sendableFunc is sendable");
} else {
    print("sendableFunc is not sendable");
}

