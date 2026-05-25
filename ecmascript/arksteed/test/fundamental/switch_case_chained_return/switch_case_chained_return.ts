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
function switch_case_chained_return(x, a, b, c, d, e)
{
    let result = a;
    switch (x) {
        case 1:
            result = result + b;
            if (result > 50) {
                return result;
            }
            result = result * 2;
            break;
        case 2:
            result = result + c;
            if (result > 50) {
                return result;
            }
            result = result * 3;
            return result;
        case 3:
            result = result + d;
            break;
        default:
            return result + e;
    }
    return result;
}

ArkTools.arkSteedCompileSync(switch_case_chained_return);


print(switch_case_chained_return(1, 10, 5, 3, 7, 100));
print(switch_case_chained_return(2, 10, 5, 3, 7, 100));
print(switch_case_chained_return(3, 10, 5, 3, 7, 100));
print(switch_case_chained_return(5, 10, 5, 3, 7, 100));