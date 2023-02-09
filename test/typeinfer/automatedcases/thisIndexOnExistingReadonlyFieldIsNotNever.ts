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

// === tests/cases/compiler/thisIndexOnExistingReadonlyFieldIsNotNever.ts ===
declare function AssertType(value:any, type:string):void;
declare class Component<P, S = {}> {
    readonly props: Readonly<{ children?: unknown }> & Readonly<P>;
    state: Readonly<S>;
}
interface CoachMarkAnchorProps<C> {
    anchorRef?: (anchor: C) => void;
}
type AnchorType<P> = Component<P>;

class CoachMarkAnchorDecorator {
    decorateComponent<P>(anchor: P) {
AssertType(class CoachMarkAnchor extends Component<CoachMarkAnchorProps<AnchorType<P>> & P, {}> {            private _onAnchorRef = (anchor: AnchorType<P>) => {                const anchorRef = this.props.anchorRef;                if (anchorRef) {                    anchorRef(anchor);                }            }        }, "typeof CoachMarkAnchor");
AssertType(CoachMarkAnchor, "typeof CoachMarkAnchor");
AssertType(Component, "Component<CoachMarkAnchorProps<AnchorType<P>> & P, {}>");
        return class CoachMarkAnchor extends Component<CoachMarkAnchorProps<AnchorType<P>> & P, {}> {

            private _onAnchorRef = (anchor: AnchorType<P>) => {
AssertType(_onAnchorRef, "(AnchorType<P>) => void");
AssertType((anchor: AnchorType<P>) => {                const anchorRef = this.props.anchorRef;                if (anchorRef) {                    anchorRef(anchor);                }            }, "(AnchorType<P>) => void");
AssertType(anchor, "AnchorType<P>");

                const anchorRef = this.props.anchorRef;
AssertType(anchorRef, "union");
AssertType(this.props.anchorRef, "union");
AssertType(this.props, "Readonly<{ children?: unknown; }> & Readonly<CoachMarkAnchorProps<AnchorType<P>> & P>");
AssertType(this, "this");

                if (anchorRef) {
AssertType(anchorRef, "union");

                    anchorRef(anchor);
AssertType(anchorRef(anchor), "void");
AssertType(anchorRef, "(AnchorType<P>) => void");
AssertType(anchor, "AnchorType<P>");
                }
            }
        };
    }
}


