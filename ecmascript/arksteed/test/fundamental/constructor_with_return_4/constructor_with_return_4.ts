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
class PointBase {
    x: number;
    y: number;

    constructor(x: number, y: number) {
        this.x = x;
        this.y = y;
        return this.innerProduct.bind(this);
    }

    innerProduct(a: number, b: number): number {
        return this.innerProductImpl(a, b);
    }
}

class Point extends PointBase {
    innerProductImpl(a: number, b: number): number {
        return this.x * a + this.y * b;
    }
}

function constructor_with_return_4() {
    return new Point(4, 5);
}

ArkTools.arkSteedCompileSync(PointBase);
ArkTools.arkSteedCompileSync(PointBase.prototype.innerProduct);
ArkTools.arkSteedCompileSync(Point);
ArkTools.arkSteedCompileSync(Point.prototype.innerProductImpl);
ArkTools.arkSteedCompileSync(constructor_with_return_4);


const p = constructor_with_return_4();
print(p(100, 10));
