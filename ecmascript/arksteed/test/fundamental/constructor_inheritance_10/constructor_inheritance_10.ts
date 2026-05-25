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
class Shape {
    x: number;
    y: number;

    constructor(x: number, y: number) {
        this.x = x;
        this.y = y;
    }

    position(): number {
        return this.x + this.y;
    }
}

class Rectangle extends Shape {
    width: number;
    height: number;

    constructor(x: number, y: number, width: number, height: number) {
        super(x, y);
        this.width = width;
        this.height = height;
    }

    area(): number {
        return this.width * this.height;
    }

    perimeter(): number {
        return 2 * (this.width + this.height);
    }
}

class Cuboid extends Rectangle {
    depth: number;

    constructor(x: number, y: number, w: number, h: number, d: number) {
        super(x, y, w, h);
        this.depth = d;
    }

    volume(): number {
        return this.width * this.height * this.depth;
    }

    surfaceArea(): number {
        return 2 * (this.width * this.height + this.height * this.depth + this.width * this.depth);
    }
}

ArkTools.arkSteedCompileSync(Shape);
ArkTools.arkSteedCompileSync(Rectangle);
ArkTools.arkSteedCompileSync(Cuboid);


let cube = new Cuboid(1, 2, 3, 4, 5);
print(cube.position());
print(cube.area());
print(cube.perimeter());
print(cube.volume());
print(cube.surfaceArea());
