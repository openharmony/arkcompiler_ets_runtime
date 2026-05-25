/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

declare function print(str: number):string;

function classifyScore(score: number): number {
    return score >= 90 ? 5 : (score >= 80 ? 4 : (score >= 70 ? 3 : (score >= 60 ? 2 : 1)));
}

function calculateBonus(base: number, years: number, performance: number): number {
    let yearsBonus = years > 5 ? base * 0.2 : (years > 3 ? base * 0.1 : 0);
    let perfBonus = performance > 80 ? base * 0.3 : (performance > 60 ? base * 0.15 : 0);
    return base + yearsBonus + perfBonus;
}

ArkTools.arkSteedCompileSync(classifyScore);
ArkTools.arkSteedCompileSync(calculateBonus);


print(classifyScore(95));
print(classifyScore(85));
print(classifyScore(75));
print(classifyScore(65));
print(classifyScore(50));
print(calculateBonus(1000, 6, 85));
print(calculateBonus(1000, 4, 70));
print(calculateBonus(1000, 2, 50));