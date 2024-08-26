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

class Var {
    x_: number;
    constructor(x: number) {
        this.x_ = x;
    }

    get getx() {
        return this.x_;
    }

    set setx(x: number) {
        this.x_ = x;
    }
}

let globalVar = new Var(0);

function func3() {
    let curVar = globalVar.getx;
    ArkTools.iterateFrame();
    globalVar.setx = curVar;
}

function func2() {
    let curVar = globalVar.getx;
    globalVar.setx = 3;
    func3();
    globalVar.setx = curVar;
}

function func1() {
    let curVar = globalVar.getx;
    globalVar.setx = 2;
    func2();
    globalVar.setx = curVar;
}

function func0() {
    let curVar = globalVar.getx;
    globalVar.setx = 1;
    func1();
    globalVar.setx = curVar;
}

print(ArkTools.isAOTCompiled(func0));
print(globalVar.getx);
func0();
print(globalVar.getx);