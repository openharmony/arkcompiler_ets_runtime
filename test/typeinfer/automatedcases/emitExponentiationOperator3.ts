/*
* Copyright (c) Microsoft Corporation. All rights reserved.
* Copyright (c) 2023 Huawei Device Co., Ltd.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/conformance/es7/exponentiationOperator/emitExponentiationOperator3.ts ===
declare function AssertType(value:any, type:string):void;
let temp = 10;
AssertType(temp, "number");
AssertType(10, "int");

(-++temp) ** 3;
AssertType((-++temp) ** 3, "number");
AssertType((-++temp), "number");
AssertType(-++temp, "number");
AssertType(++temp, "number");
AssertType(temp, "number");
AssertType(3, "int");

(+--temp) ** 3;
AssertType((+--temp) ** 3, "number");
AssertType((+--temp), "number");
AssertType(+--temp, "number");
AssertType(--temp, "number");
AssertType(temp, "number");
AssertType(3, "int");

(-temp++) ** 3;
AssertType((-temp++) ** 3, "number");
AssertType((-temp++), "number");
AssertType(-temp++, "number");
AssertType(temp++, "number");
AssertType(temp, "number");
AssertType(3, "int");

(+temp--) ** 3;
AssertType((+temp--) ** 3, "number");
AssertType((+temp--), "number");
AssertType(+temp--, "number");
AssertType(temp--, "number");
AssertType(temp, "number");
AssertType(3, "int");

(-(1 ** ++temp)) ** 3;
AssertType((-(1 ** ++temp)) ** 3, "number");
AssertType((-(1 ** ++temp)), "number");
AssertType(-(1 ** ++temp), "number");
AssertType((1 ** ++temp), "number");
AssertType(1 ** ++temp, "number");
AssertType(1, "int");
AssertType(++temp, "number");
AssertType(temp, "number");
AssertType(3, "int");

(-(1 ** --temp)) ** 3;
AssertType((-(1 ** --temp)) ** 3, "number");
AssertType((-(1 ** --temp)), "number");
AssertType(-(1 ** --temp), "number");
AssertType((1 ** --temp), "number");
AssertType(1 ** --temp, "number");
AssertType(1, "int");
AssertType(--temp, "number");
AssertType(temp, "number");
AssertType(3, "int");

(-(1 ** temp++)) ** 3;
AssertType((-(1 ** temp++)) ** 3, "number");
AssertType((-(1 ** temp++)), "number");
AssertType(-(1 ** temp++), "number");
AssertType((1 ** temp++), "number");
AssertType(1 ** temp++, "number");
AssertType(1, "int");
AssertType(temp++, "number");
AssertType(temp, "number");
AssertType(3, "int");

(-(1 ** temp--)) ** 3;
AssertType((-(1 ** temp--)) ** 3, "number");
AssertType((-(1 ** temp--)), "number");
AssertType(-(1 ** temp--), "number");
AssertType((1 ** temp--), "number");
AssertType(1 ** temp--, "number");
AssertType(1, "int");
AssertType(temp--, "number");
AssertType(temp, "number");
AssertType(3, "int");

(-3) ** temp++;
AssertType((-3) ** temp++, "number");
AssertType((-3), "int");
AssertType(-3, "int");
AssertType(3, "int");
AssertType(temp++, "number");
AssertType(temp, "number");

(-3) ** temp--;
AssertType((-3) ** temp--, "number");
AssertType((-3), "int");
AssertType(-3, "int");
AssertType(3, "int");
AssertType(temp--, "number");
AssertType(temp, "number");

(-3) ** ++temp;
AssertType((-3) ** ++temp, "number");
AssertType((-3), "int");
AssertType(-3, "int");
AssertType(3, "int");
AssertType(++temp, "number");
AssertType(temp, "number");

(-3) ** --temp;
AssertType((-3) ** --temp, "number");
AssertType((-3), "int");
AssertType(-3, "int");
AssertType(3, "int");
AssertType(--temp, "number");
AssertType(temp, "number");

(+3) ** temp++;
AssertType((+3) ** temp++, "number");
AssertType((+3), "int");
AssertType(+3, "int");
AssertType(3, "int");
AssertType(temp++, "number");
AssertType(temp, "number");

(+3) ** temp--;
AssertType((+3) ** temp--, "number");
AssertType((+3), "int");
AssertType(+3, "int");
AssertType(3, "int");
AssertType(temp--, "number");
AssertType(temp, "number");

(+3) ** ++temp;
AssertType((+3) ** ++temp, "number");
AssertType((+3), "int");
AssertType(+3, "int");
AssertType(3, "int");
AssertType(++temp, "number");
AssertType(temp, "number");

(+3) ** --temp;
AssertType((+3) ** --temp, "number");
AssertType((+3), "int");
AssertType(+3, "int");
AssertType(3, "int");
AssertType(--temp, "number");
AssertType(temp, "number");

(-3) ** temp++ ** 2;
AssertType((-3) ** temp++ ** 2, "number");
AssertType((-3), "int");
AssertType(-3, "int");
AssertType(3, "int");
AssertType(temp++ ** 2, "number");
AssertType(temp++, "number");
AssertType(temp, "number");
AssertType(2, "int");

(-3) ** temp-- ** 2;
AssertType((-3) ** temp-- ** 2, "number");
AssertType((-3), "int");
AssertType(-3, "int");
AssertType(3, "int");
AssertType(temp-- ** 2, "number");
AssertType(temp--, "number");
AssertType(temp, "number");
AssertType(2, "int");

(-3) ** ++temp ** 2;
AssertType((-3) ** ++temp ** 2, "number");
AssertType((-3), "int");
AssertType(-3, "int");
AssertType(3, "int");
AssertType(++temp ** 2, "number");
AssertType(++temp, "number");
AssertType(temp, "number");
AssertType(2, "int");

(-3) ** --temp ** 2;
AssertType((-3) ** --temp ** 2, "number");
AssertType((-3), "int");
AssertType(-3, "int");
AssertType(3, "int");
AssertType(--temp ** 2, "number");
AssertType(--temp, "number");
AssertType(temp, "number");
AssertType(2, "int");

(+3) ** temp++ ** 2;
AssertType((+3) ** temp++ ** 2, "number");
AssertType((+3), "int");
AssertType(+3, "int");
AssertType(3, "int");
AssertType(temp++ ** 2, "number");
AssertType(temp++, "number");
AssertType(temp, "number");
AssertType(2, "int");

(+3) ** temp-- ** 2;
AssertType((+3) ** temp-- ** 2, "number");
AssertType((+3), "int");
AssertType(+3, "int");
AssertType(3, "int");
AssertType(temp-- ** 2, "number");
AssertType(temp--, "number");
AssertType(temp, "number");
AssertType(2, "int");

(+3) ** ++temp ** 2;
AssertType((+3) ** ++temp ** 2, "number");
AssertType((+3), "int");
AssertType(+3, "int");
AssertType(3, "int");
AssertType(++temp ** 2, "number");
AssertType(++temp, "number");
AssertType(temp, "number");
AssertType(2, "int");

(+3) ** --temp ** 2;
AssertType((+3) ** --temp ** 2, "number");
AssertType((+3), "int");
AssertType(+3, "int");
AssertType(3, "int");
AssertType(--temp ** 2, "number");
AssertType(--temp, "number");
AssertType(temp, "number");
AssertType(2, "int");

3 ** -temp++;
AssertType(3 ** -temp++, "number");
AssertType(3, "int");
AssertType(-temp++, "number");
AssertType(temp++, "number");
AssertType(temp, "number");

3 ** -temp--;
AssertType(3 ** -temp--, "number");
AssertType(3, "int");
AssertType(-temp--, "number");
AssertType(temp--, "number");
AssertType(temp, "number");

3 ** -++temp;
AssertType(3 ** -++temp, "number");
AssertType(3, "int");
AssertType(-++temp, "number");
AssertType(++temp, "number");
AssertType(temp, "number");

3 ** +--temp;
AssertType(3 ** +--temp, "number");
AssertType(3, "int");
AssertType(+--temp, "number");
AssertType(--temp, "number");
AssertType(temp, "number");

3 ** (-temp++) ** 2;
AssertType(3 ** (-temp++) ** 2, "number");
AssertType(3, "int");
AssertType((-temp++) ** 2, "number");
AssertType((-temp++), "number");
AssertType(-temp++, "number");
AssertType(temp++, "number");
AssertType(temp, "number");
AssertType(2, "int");

3 ** (-temp--) ** 2;
AssertType(3 ** (-temp--) ** 2, "number");
AssertType(3, "int");
AssertType((-temp--) ** 2, "number");
AssertType((-temp--), "number");
AssertType(-temp--, "number");
AssertType(temp--, "number");
AssertType(temp, "number");
AssertType(2, "int");

3 ** (+temp++) ** 2;
AssertType(3 ** (+temp++) ** 2, "number");
AssertType(3, "int");
AssertType((+temp++) ** 2, "number");
AssertType((+temp++), "number");
AssertType(+temp++, "number");
AssertType(temp++, "number");
AssertType(temp, "number");
AssertType(2, "int");

3 ** (+temp--) ** 2;
AssertType(3 ** (+temp--) ** 2, "number");
AssertType(3, "int");
AssertType((+temp--) ** 2, "number");
AssertType((+temp--), "number");
AssertType(+temp--, "number");
AssertType(temp--, "number");
AssertType(temp, "number");
AssertType(2, "int");

3 ** (-++temp) ** 2;
AssertType(3 ** (-++temp) ** 2, "number");
AssertType(3, "int");
AssertType((-++temp) ** 2, "number");
AssertType((-++temp), "number");
AssertType(-++temp, "number");
AssertType(++temp, "number");
AssertType(temp, "number");
AssertType(2, "int");

3 ** (+--temp) ** 2;
AssertType(3 ** (+--temp) ** 2, "number");
AssertType(3, "int");
AssertType((+--temp) ** 2, "number");
AssertType((+--temp), "number");
AssertType(+--temp, "number");
AssertType(--temp, "number");
AssertType(temp, "number");
AssertType(2, "int");


