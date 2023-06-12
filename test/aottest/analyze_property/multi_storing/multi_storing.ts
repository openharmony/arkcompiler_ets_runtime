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

declare function print(arg: any): string;
declare class ArkTools {
    static isNotHoleProperty(obj: object, key: string): boolean;
}

class TestLoad {
    x: number = 1;
    y: string;
    constructor() {
        this.y = "a1";
        print(this.y);
        this.y = "a2";
    }
}

class TestEscapeThis {
    x: number = 1;
    y: string;
    constructor() {
        this.y = "b1";
        // @ts-ignore
        foo(this);
        this.y = "b2";
    }
}

function foo(obj: TestEscapeThis) {
    print(obj.y);
}

function test(): void {
    let a = new TestLoad();
    print(ArkTools.isNotHoleProperty(a, "x"));
    print(ArkTools.isNotHoleProperty(a, "y"));

    let b = new TestEscapeThis();
    print(ArkTools.isNotHoleProperty(a, "x"));
    print(ArkTools.isNotHoleProperty(a, "y"));
}

test();

