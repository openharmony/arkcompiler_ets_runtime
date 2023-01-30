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

// === tests/cases/compiler/trivialSubtypeReductionNoStructuralCheck2.ts ===
declare function AssertType(value:any, type:string):void;
declare const props: WizardStepProps;
AssertType(props, "WizardStepProps");

export class Wizard {
  get steps() {
AssertType({      wizard: this as Wizard,      ...props,    } as WizardStepProps, "WizardStepProps");
AssertType({      wizard: this as Wizard,      ...props,    }, "{ wizard: Wizard; }");
    return {

      wizard: this as Wizard,
AssertType(wizard, "Wizard");
AssertType(this as Wizard, "Wizard");
AssertType(this, "this");

      ...props,
AssertType(props, "WizardStepProps");

    } as WizardStepProps;
  }
}

export interface WizardStepProps {
  wizard?: Wizard;
}

