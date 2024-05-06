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
 * @tc.name:sendablecontext
 * @tc.desc:test sendablecontext
 * @tc.type: FUNC
 * @tc.require: issue#I8SFAK
 */

// @ts-nocheck
declare function print(str: any): string;

class SendableClassA {
    static staticField: string = "staticField";
    constructor() {
        "use sendable";
    }

    testZeroLevelContext() {
        print(SendableClassA.staticField);
    }

    testOneLevelContext() {
        let x: number = 0;
        (()=>{
            print(SendableClassA.staticField);
            print("closureVariable x = " + x);
        })();
    }
    
    testTwoLevelContext() {
        let y: number = 0;
        class InnerClass {
            static innerStaticField: string = "innerStaticField";
            innerFunc() {
                let x: number = 0;
                (()=>{
                    print(SendableClassA.staticField);
                    print("closureVariable y = " + y);
                    print("closureVariable x = " + x);
                    print("innerStaticField = " + InnerClass.innerStaticField);
                })();
            }
        }
        let innerObj = new InnerClass;
        innerObj.innerFunc();
    }
}

let sObj = new SendableClassA;
sObj.testZeroLevelContext();
sObj.testOneLevelContext();
sObj.testTwoLevelContext();

class SendableClassB extends SendableClassA {
    x:number = 5;
    constructor() {
        "use sendable";
        super();
    }

    testZeroLevelContext() {
        print(SendableClassB.staticField);
    }
}

let sObjb = new SendableClassB;
sObjb.testZeroLevelContext()
