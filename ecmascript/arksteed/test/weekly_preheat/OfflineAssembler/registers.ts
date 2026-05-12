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

'use strict';

let gprs = [
  't0', 't1', 't2', 't3', 't4', 't5',
  'cfr', 'a0', 'a1', 'a2', 'a3', 'r0',
  'r1', 'sp', 'lr', 'pc',
  // 64-bit only registers:
  'csr0', 'csr1', 'csr2', 'csr3', 'csr4',
  'csr5', 'csr6', 'csr7', 'csr8', 'csr9'
];

let fprs = [
  'ft0', 'ft1', 'ft2', 'ft3', 'ft4', 'ft5',
  'fa0', 'fa1', 'fa2', 'fa3',
  'csfr0', 'csfr1', 'csfr2', 'csfr3', 'csfr4', 'csfr5', 'csfr6', 'csfr7',
  'fr'
];

let registers = gprs.concat(fprs);

let gprPattern = new RegExp('^((' + gprs.join(')|(') + '))$');
export let fprPattern = new RegExp('^((' + fprs.join(')|(') + '))$');
export let registerPattern = new RegExp(
  '^((' + registers.join(')|(') + '))$'
);
