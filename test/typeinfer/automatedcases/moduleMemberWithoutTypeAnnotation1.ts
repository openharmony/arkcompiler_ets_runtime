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

// === tests/cases/compiler/moduleMemberWithoutTypeAnnotation1.ts ===
declare function AssertType(value:any, type:string):void;
module TypeScript.Parser {
    class SyntaxCursor {
        public currentNode(): SyntaxNode {
AssertType(null, "null");
            return null;
        }
    }
}

module TypeScript {
    export interface ISyntaxElement { };
    export interface ISyntaxToken { };

    export class PositionedElement {
        public childIndex(child: ISyntaxElement) {
AssertType(Syntax.childIndex(), "void");
AssertType(Syntax.childIndex, "() => void");
            return Syntax.childIndex();
        }
    }

    export class PositionedToken {
        constructor(parent: PositionedElement, token: ISyntaxToken, fullStart: number) {
        }
    }
}

module TypeScript {
    export class SyntaxNode {
        public findToken(position: number, includeSkippedTokens: boolean = false): PositionedToken {
            let positionedToken = this.findTokenInternal(null, position, 0);
AssertType(positionedToken, "any");
AssertType(this.findTokenInternal(null, position, 0), "any");
AssertType(this.findTokenInternal, "(any, any, any) => any");
AssertType(this, "this");
AssertType(null, "null");
AssertType(position, "number");
AssertType(0, "int");

AssertType(null, "null");
            return null;
        }
        findTokenInternal(x, y, z) {
AssertType(null, "null");
            return null;
        }
    }
}

module TypeScript.Syntax {
    export function childIndex() { }

    export class VariableWidthTokenWithTrailingTrivia implements ISyntaxToken {
        private findTokenInternal(parent: PositionedElement, position: number, fullStart: number) {
AssertType(new PositionedToken(parent, this, fullStart), "PositionedToken");
AssertType(PositionedToken, "typeof PositionedToken");
AssertType(parent, "PositionedElement");
AssertType(this, "this");
AssertType(fullStart, "number");
            return new PositionedToken(parent, this, fullStart);
        }
    }
}


