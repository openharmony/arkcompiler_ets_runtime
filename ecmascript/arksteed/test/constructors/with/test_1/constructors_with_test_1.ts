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
class Point {
    constructor(x: number, y: number) {
        return [x, y];
    }
}

function constructor_with_return_1() {
    return new Point(4, 5);
}

ArkTools.arkSteedCompileSync(Point);
ArkTools.arkSteedCompileSync(constructor_with_return_1);


const p = constructor_with_return_1();
print(p instanceof Array);
