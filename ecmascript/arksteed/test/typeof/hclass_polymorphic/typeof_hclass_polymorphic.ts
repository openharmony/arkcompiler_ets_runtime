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

//! METHOD      typeof_poly_agree
//! HAS_NOT     CallCommonStub TypeOf
//! METHOD      typeof_poly_disagree
//! HAS         CallCommonStub TypeOf
// Test: typeof on a polymorphic receiver is only foldable when all possible
// HClasses agree. Shapes A and B agree on "object" and typeof_poly_agree is
// folded; adding the callable shape diverges ("object" vs "function"), so
// typeof_poly_disagree keeps the TypeOf stub call and stays correct at runtime.

declare function print(arg: number): string;

declare class ArkTools {
    static arkSteedCompileSync<T extends Function>(func: T): T;
}

class PolyShapeA {
    x: number = 1;
    padA: number = 2;
}

class PolyShapeB {
    padB: number = 3;
    x: number = 4;
}

function typeof_poly_agree(o: any): number {
    let v = o.x;
    let result = typeof o === "object" ? 1 : 0;
    return result + v;
}

function typeof_poly_disagree(o: any): number {
    let v = o.x;
    let result = typeof o === "object" ? 1 : 0;
    return result + v;
}

typeof_poly_agree(new PolyShapeA());
typeof_poly_agree(new PolyShapeB());

ArkTools.arkSteedCompileSync(typeof_poly_agree);

print(typeof_poly_agree(new PolyShapeA()));
print(typeof_poly_agree(new PolyShapeB()));

let fnShape: any = function () { };
fnShape.x = 5;

typeof_poly_disagree(new PolyShapeA());
typeof_poly_disagree(fnShape);

ArkTools.arkSteedCompileSync(typeof_poly_disagree);

print(typeof_poly_disagree(new PolyShapeA()));
print(typeof_poly_disagree(fnShape));
