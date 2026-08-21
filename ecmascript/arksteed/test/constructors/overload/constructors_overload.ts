/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
class Rectangle {
    width: number;
    height: number;

    constructor();
    constructor(width: number);
    constructor(width: number, height: number);
    constructor(width?: number, height?: number) {
        this.width = width || 1;
        this.height = height || this.width;
    }

    area(): number {
        return this.width * this.height;
    }
}

ArkTools.arkSteedCompileSync(Rectangle);


let rect1 = new Rectangle();
print(rect1.width);
print(rect1.height);
print(rect1.area());

let rect2 = new Rectangle(5);
print(rect2.width);
print(rect2.height);
print(rect2.area());

let rect3 = new Rectangle(4, 6);
print(rect3.width);
print(rect3.height);
print(rect3.area());