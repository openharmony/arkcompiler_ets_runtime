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
 * @tc.name:sharedmodule
 * @tc.desc:test sharedmodule
 * @tc.type: FUNC
 * @tc.require: issue#I9BIE5
 */

// @ts-nocheck
declare function print(str: any): string;

import {Test1} from "./func"
import {strA, strB, foo} from "./string"

"shared module"
class SendableClassA {
    static staticField: string = strA;
    constructor() {
        "use sendable";
    }

    testZeroLevelContext() {
        print(SendableClassA.staticField);
    }

    testOneLevelContext() {
        let x: number = 0;
        (()=>{
            print("testOneLevelContext");
            print(SendableClassA.staticField + strB);
        })();
    }
    testImportFunction() {
        let x: number = 0;
        (()=>{
            print("testImportFunction");
            let tA = new Test1(strA);
            print(tA.foo());
        })();
    }
}

let sObj = new SendableClassA;
sObj.testZeroLevelContext();
sObj.testOneLevelContext();
sObj.testImportFunction();

class SendableClassB extends SendableClassA {
    classBStr:string;
    constructor(classBStr:string) {
        "use sendable";
        super();
        this.classBStr = classBStr;
    }

    testImportFunction() {
        print("testImportFunction");
        print(this.classBStr);
    }
}

let sObjb = new SendableClassB("ClassB:");
sObjb.testImportFunction()
foo();
