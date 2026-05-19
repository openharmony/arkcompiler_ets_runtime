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
function switch_true_chained_conditions(score, bonus, a, b, c, d)
{
    let result = 0;
    switch (true) {
        case score >= 90:
            result = a;
            if (bonus > 0) {
                result = result + bonus * 2;
            }
            break;
        case score >= 80:
            result = b;
            if (bonus > 5) {
                result = result + bonus;
            }
            break;
        case score >= 70:
            result = c;
            if (bonus > 10) {
                result = result + bonus / 2;
            }
            break;
        default:
            result = d;
            break;
    }
    return result;
}

ArkTools.arkSteedCompileSync(switch_true_chained_conditions);


print(switch_true_chained_conditions(95, 5, 100, 80, 60, 40));
print(switch_true_chained_conditions(95, 0, 100, 80, 60, 40));
print(switch_true_chained_conditions(85, 10, 100, 80, 60, 40));
print(switch_true_chained_conditions(75, 15, 100, 80, 60, 40));
print(switch_true_chained_conditions(65, 0, 100, 80, 60, 40));
