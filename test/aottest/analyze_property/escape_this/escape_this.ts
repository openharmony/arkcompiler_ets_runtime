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

class C {
    constructor() {
        // @ts-ignore
        foo(this);
    }
}

class D extends C {
    x: number = 1;
    constructor() {
        super();
    }
}

function foo(obj: D) {
    print(obj.x);
}

function test(): void {
    let d = new D();
    print(ArkTools.isNotHoleProperty(d, "x"));
}

test();
