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
function switch_true_nested(x, y, a, b, c, d)
{
    switch (true) {
        case x > 0:
            if (y > 0) {
                return a + b;
            } else {
                return a - b;
            }
        case x < 0:
            switch (true) {
                case y > 0:
                    return c + d;
                case y <= 0:
                    return c - d;
            }
        default:
            return a + c;
    }
}

ArkTools.arkSteedCompileSync(switch_true_nested);


print(switch_true_nested(5, 3, 10, 20, 30, 40));
print(switch_true_nested(5, -3, 10, 20, 30, 40));
print(switch_true_nested(-5, 3, 10, 20, 30, 40));
print(switch_true_nested(-5, -3, 10, 20, 30, 40));
print(switch_true_nested(0, 3, 10, 20, 30, 40));

print(switch_true_nested(5, 3, 1, 20, 300, 4000));
print(switch_true_nested(5, -3, 1, 20, 300, 4000));
print(switch_true_nested(-5, 3, 1, 20, 300, 4000));
print(switch_true_nested(-5, -3, 1, 20, 300, 4000));
print(switch_true_nested(0, 3, 1, 20, 300, 4000));
