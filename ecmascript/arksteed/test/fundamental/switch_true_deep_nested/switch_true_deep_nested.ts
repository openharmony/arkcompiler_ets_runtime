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
function switch_true_deep_nested(x, y, z, a, b, c, d)
{
    let result = 0;
    switch (true) {
        case x > 0:
            switch (true) {
                case y > 0:
                    switch (true) {
                        case z > 0:
                            result = a + b + c;
                            break;
                        case z <= 0:
                            result = a + b - c;
                            break;
                    }
                    break;
                case y <= 0:
                    result = a + d;
                    break;
            }
            break;
        case x <= 0:
            switch (true) {
                case y > 0 && z > 0:
                    result = b + c;
                    break;
                default:
                    result = c + d;
                    break;
            }
            break;
    }
    return result;
}

ArkTools.arkSteedCompileSync(switch_true_deep_nested);


print(switch_true_deep_nested(5, 3, 2, 10, 20, 30, 40));
print(switch_true_deep_nested(5, 3, -2, 10, 20, 30, 40));
print(switch_true_deep_nested(5, -3, 2, 10, 20, 30, 40));
print(switch_true_deep_nested(-5, 3, 2, 10, 20, 30, 40));
print(switch_true_deep_nested(-5, -3, 2, 10, 20, 30, 40));
print(switch_true_deep_nested(-5, 3, -2, 10, 20, 30, 40));

print(switch_true_deep_nested(5, 3, 2, 1, 20, 300, 4000));
print(switch_true_deep_nested(5, 3, -2, 1, 20, 300, 4000));
print(switch_true_deep_nested(5, -3, 2, 1, 20, 300, 4000));
print(switch_true_deep_nested(-5, 3, 2, 1, 20, 300, 4000));
print(switch_true_deep_nested(-5, -3, 2, 1, 20, 300, 4000));
print(switch_true_deep_nested(-5, 3, -2, 1, 20, 300, 4000));
