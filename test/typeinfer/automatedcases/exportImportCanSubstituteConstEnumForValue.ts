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

// === tests/cases/compiler/exportImportCanSubstituteConstEnumForValue.ts ===
declare function AssertType(value:any, type:string):void;
module MsPortalFx.ViewModels.Dialogs {

    export const enum DialogResult {
        Abort,
        Cancel,
        Ignore,
        No,
        Ok,
        Retry,
        Yes,
    }

    export interface DialogResultCallback {
        (result: MsPortalFx.ViewModels.Dialogs.DialogResult): void;
    }

    export function someExportedFunction() {
    }

    export const enum MessageBoxButtons {
        AbortRetryIgnore,
        OK,
        OKCancel,
        RetryCancel,
        YesNo,
        YesNoCancel,
    }
}


module MsPortalFx.ViewModels {

    /**
     * For some reason javascript code is emitted for this re-exported const enum.
     */
    export import ReExportedEnum = Dialogs.DialogResult;

    /**
     * Not exported to show difference. No javascript is emmitted (as expected)
     */
    import DialogButtons = Dialogs.MessageBoxButtons;

    /**
     * Re-exporting a function type to show difference. No javascript is emmitted (as expected)
     */
    export import Callback = Dialogs.DialogResultCallback;

    export class SomeUsagesOfTheseConsts {
        constructor() {
            // these do get replaced by the const value
            const value1 = ReExportedEnum.Cancel;
AssertType(value1, "ReExportedEnum.Cancel");
AssertType(ReExportedEnum.Cancel, "ReExportedEnum.Cancel");

            console.log(value1);
AssertType(console.log(value1), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(value1, "ReExportedEnum.Cancel");

            const value2 = DialogButtons.OKCancel;
AssertType(value2, "DialogButtons.OKCancel");
AssertType(DialogButtons.OKCancel, "DialogButtons.OKCancel");

            console.log(value2);
AssertType(console.log(value2), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(value2, "DialogButtons.OKCancel");
        }
    }
}


