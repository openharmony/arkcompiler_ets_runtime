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
 * @tc.name:sendableclassuseimport
 * @tc.desc:test sendableclassuseimport
 * @tc.type: FUNC
 * @tc.require: issue#IB6QMB
 */

// @ts-nocheck
declare function print(str: any): string;
export class SendableLocalClass {
    constructor() {
        "use sendable";
    }
    func(tag : string) {
        print("SendableLocalClass, " + tag);
    }
}

export function sendableLocalFunc(tag : string) {
    "use sendable";
    print("sendableLocalFunc, " + tag);
}
export function localFunc(tag : string) {
    print("localFunc, " + tag);
}
export class LocalClass {
    func(tag : string) {
        print("LocalClass, " + tag);
    }
}
export class SendableUseLocalModuleValue {
    constructor() {
        "use sendable";
    }
    start() {
        new SendableLocalClass().func("SendableUseLocalModuleValue::start");
        sendableLocalFunc("SendableUseLocalModuleValue::start");
        new LocalClass().func("SendableUseLocalModuleValue::start");
        localFunc("SendableUseLocalModuleValue::start");
    }
    static {
        new SendableLocalClass().func("SendableUseLocalModuleValue::static block");
        sendableLocalFunc("SendableUseLocalModuleValue::static block");
        new LocalClass().func("SendableUseLocalModuleValue::static block");
        localFunc("SendableUseLocalModuleValue::static block");
        class InStaticSendableClass {
            constructor() {
                "use sendable";
            }
            foo() {
                new SendableLocalClass().func("InStaticSendableClass::foo");
                sendableLocalFunc("InStaticSendableClass::foo");
                new LocalClass().func("InStaticSendableClass::foo");
                localFunc("InStaticSendableClass::foo");
            }
        }
        new InStaticSendableClass().foo()
        class InStaticClass {
            foo() {
                new SendableLocalClass().func("InStaticClass::foo");
                sendableLocalFunc("InStaticClass::foo");
                new LocalClass().func("InStaticClass::foo");
                localFunc("InStaticClass::foo");
            }
        }
        new InStaticClass().foo()
    }
}

export function sendableFuncUseLocalModuleValue() {
    "use sendable";
    new SendableLocalClass().func("SendableFuncUseLocalModuleValue");
    sendableLocalFunc("SendableFuncUseLocalModuleValue");
    new LocalClass().func("SendableFuncUseLocalModuleValue");
    localFunc("SendableFuncUseLocalModuleValue");
}