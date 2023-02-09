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

// === tests/cases/compiler/nonNullReferenceMatching.ts ===
declare function AssertType(value:any, type:string):void;
type ElementRef = (element: HTMLElement | null) => void;

type ThumbProps = {
    elementRef?: ElementRef;
}

type ComponentProps = {
    thumbYProps?: ThumbProps;
    thumbXProps: ThumbProps;
}

class Component {
    props!: ComponentProps;
    public thumbYElementRef = (ref: HTMLElement | null) => {
        typeof this.props.thumbYProps!.elementRef === 'function' && this.props.thumbYProps!.elementRef(ref);
AssertType(typeof this.props.thumbYProps!.elementRef === 'function' && this.props.thumbYProps!.elementRef(ref), "union");
AssertType(typeof this.props.thumbYProps!.elementRef === 'function', "boolean");
AssertType(typeof this.props.thumbYProps!.elementRef, "union");
AssertType(this.props.thumbYProps!.elementRef, "union");
AssertType(this.props.thumbYProps!, "ThumbProps");
AssertType(this.props.thumbYProps, "union");
AssertType(this.props, "ComponentProps");
AssertType(this, "this");
AssertType('function', "string");
AssertType(this.props.thumbYProps!.elementRef(ref), "void");
AssertType(this.props.thumbYProps!.elementRef, "ElementRef");
AssertType(this.props.thumbYProps!, "ThumbProps");
AssertType(this.props.thumbYProps, "union");
AssertType(this.props, "ComponentProps");
AssertType(this, "this");
AssertType(ref, "union");

        typeof (this.props.thumbYProps!.elementRef) === 'function' && this.props.thumbYProps!.elementRef(ref);
AssertType(typeof (this.props.thumbYProps!.elementRef) === 'function' && this.props.thumbYProps!.elementRef(ref), "union");
AssertType(typeof (this.props.thumbYProps!.elementRef) === 'function', "boolean");
AssertType(typeof (this.props.thumbYProps!.elementRef), "union");
AssertType((this.props.thumbYProps!.elementRef), "union");
AssertType(this.props.thumbYProps!.elementRef, "union");
AssertType(this.props.thumbYProps!, "ThumbProps");
AssertType(this.props.thumbYProps, "union");
AssertType(this.props, "ComponentProps");
AssertType(this, "this");
AssertType('function', "string");
AssertType(this.props.thumbYProps!.elementRef(ref), "void");
AssertType(this.props.thumbYProps!.elementRef, "ElementRef");
AssertType(this.props.thumbYProps!, "ThumbProps");
AssertType(this.props.thumbYProps, "union");
AssertType(this.props, "ComponentProps");
AssertType(this, "this");
AssertType(ref, "union");

        typeof ((this.props).thumbYProps!.elementRef)! === 'function' && this.props.thumbYProps!.elementRef(ref);
AssertType(typeof ((this.props).thumbYProps!.elementRef)! === 'function' && this.props.thumbYProps!.elementRef(ref), "union");
AssertType(typeof ((this.props).thumbYProps!.elementRef)! === 'function', "boolean");
AssertType(typeof ((this.props).thumbYProps!.elementRef)!, "union");
AssertType(((this.props).thumbYProps!.elementRef)!, "ElementRef");
AssertType(((this.props).thumbYProps!.elementRef), "union");
AssertType((this.props).thumbYProps!.elementRef, "union");
AssertType((this.props).thumbYProps!, "ThumbProps");
AssertType((this.props).thumbYProps, "union");
AssertType((this.props), "ComponentProps");
AssertType(this.props, "ComponentProps");
AssertType(this, "this");
AssertType('function', "string");
AssertType(this.props.thumbYProps!.elementRef(ref), "void");
AssertType(this.props.thumbYProps!.elementRef, "ElementRef");
AssertType(this.props.thumbYProps!, "ThumbProps");
AssertType(this.props.thumbYProps, "union");
AssertType(this.props, "ComponentProps");
AssertType(this, "this");
AssertType(ref, "union");

        typeof this.props.thumbXProps.elementRef === 'function' && this.props.thumbXProps.elementRef(ref);
AssertType(typeof this.props.thumbXProps.elementRef === 'function' && this.props.thumbXProps.elementRef(ref), "union");
AssertType(typeof this.props.thumbXProps.elementRef === 'function', "boolean");
AssertType(typeof this.props.thumbXProps.elementRef, "union");
AssertType(this.props.thumbXProps.elementRef, "union");
AssertType(this.props.thumbXProps, "ThumbProps");
AssertType(this.props, "ComponentProps");
AssertType(this, "this");
AssertType('function', "string");
AssertType(this.props.thumbXProps.elementRef(ref), "void");
AssertType(this.props.thumbXProps.elementRef, "ElementRef");
AssertType(this.props.thumbXProps, "ThumbProps");
AssertType(this.props, "ComponentProps");
AssertType(this, "this");
AssertType(ref, "union");

        typeof this.props.thumbXProps.elementRef === 'function' && (this.props).thumbXProps.elementRef(ref);
AssertType(typeof this.props.thumbXProps.elementRef === 'function' && (this.props).thumbXProps.elementRef(ref), "union");
AssertType(typeof this.props.thumbXProps.elementRef === 'function', "boolean");
AssertType(typeof this.props.thumbXProps.elementRef, "union");
AssertType(this.props.thumbXProps.elementRef, "union");
AssertType(this.props.thumbXProps, "ThumbProps");
AssertType(this.props, "ComponentProps");
AssertType(this, "this");
AssertType('function', "string");
AssertType((this.props).thumbXProps.elementRef(ref), "void");
AssertType((this.props).thumbXProps.elementRef, "ElementRef");
AssertType((this.props).thumbXProps, "ThumbProps");
AssertType((this.props), "ComponentProps");
AssertType(this.props, "ComponentProps");
AssertType(this, "this");
AssertType(ref, "union");

        typeof this.props.thumbXProps.elementRef === 'function' && (this.props.thumbXProps).elementRef(ref);
AssertType(typeof this.props.thumbXProps.elementRef === 'function' && (this.props.thumbXProps).elementRef(ref), "union");
AssertType(typeof this.props.thumbXProps.elementRef === 'function', "boolean");
AssertType(typeof this.props.thumbXProps.elementRef, "union");
AssertType(this.props.thumbXProps.elementRef, "union");
AssertType(this.props.thumbXProps, "ThumbProps");
AssertType(this.props, "ComponentProps");
AssertType(this, "this");
AssertType('function', "string");
AssertType((this.props.thumbXProps).elementRef(ref), "void");
AssertType((this.props.thumbXProps).elementRef, "ElementRef");
AssertType((this.props.thumbXProps), "ThumbProps");
AssertType(this.props.thumbXProps, "ThumbProps");
AssertType(this.props, "ComponentProps");
AssertType(this, "this");
AssertType(ref, "union");

        typeof this.props.thumbXProps.elementRef === 'function' && ((this.props)!.thumbXProps)!.elementRef(ref);
AssertType(typeof this.props.thumbXProps.elementRef === 'function' && ((this.props)!.thumbXProps)!.elementRef(ref), "union");
AssertType(typeof this.props.thumbXProps.elementRef === 'function', "boolean");
AssertType(typeof this.props.thumbXProps.elementRef, "union");
AssertType(this.props.thumbXProps.elementRef, "union");
AssertType(this.props.thumbXProps, "ThumbProps");
AssertType(this.props, "ComponentProps");
AssertType(this, "this");
AssertType('function', "string");
AssertType(((this.props)!.thumbXProps)!.elementRef(ref), "void");
AssertType(((this.props)!.thumbXProps)!.elementRef, "ElementRef");
AssertType(((this.props)!.thumbXProps)!, "ThumbProps");
AssertType(((this.props)!.thumbXProps), "ThumbProps");
AssertType((this.props)!.thumbXProps, "ThumbProps");
AssertType((this.props)!, "ComponentProps");
AssertType((this.props), "ComponentProps");
AssertType(this.props, "ComponentProps");
AssertType(this, "this");
AssertType(ref, "union");

        typeof (this.props.thumbXProps).elementRef === 'function' && ((this.props)!.thumbXProps)!.elementRef(ref);
AssertType(typeof (this.props.thumbXProps).elementRef === 'function' && ((this.props)!.thumbXProps)!.elementRef(ref), "union");
AssertType(typeof (this.props.thumbXProps).elementRef === 'function', "boolean");
AssertType(typeof (this.props.thumbXProps).elementRef, "union");
AssertType((this.props.thumbXProps).elementRef, "union");
AssertType((this.props.thumbXProps), "ThumbProps");
AssertType(this.props.thumbXProps, "ThumbProps");
AssertType(this.props, "ComponentProps");
AssertType(this, "this");
AssertType('function', "string");
AssertType(((this.props)!.thumbXProps)!.elementRef(ref), "void");
AssertType(((this.props)!.thumbXProps)!.elementRef, "ElementRef");
AssertType(((this.props)!.thumbXProps)!, "ThumbProps");
AssertType(((this.props)!.thumbXProps), "ThumbProps");
AssertType((this.props)!.thumbXProps, "ThumbProps");
AssertType((this.props)!, "ComponentProps");
AssertType((this.props), "ComponentProps");
AssertType(this.props, "ComponentProps");
AssertType(this, "this");
AssertType(ref, "union");

        typeof this.props!.thumbXProps!.elementRef === 'function' && ((this.props)!.thumbXProps)!.elementRef(ref);
AssertType(typeof this.props!.thumbXProps!.elementRef === 'function' && ((this.props)!.thumbXProps)!.elementRef(ref), "union");
AssertType(typeof this.props!.thumbXProps!.elementRef === 'function', "boolean");
AssertType(typeof this.props!.thumbXProps!.elementRef, "union");
AssertType(this.props!.thumbXProps!.elementRef, "union");
AssertType(this.props!.thumbXProps!, "ThumbProps");
AssertType(this.props!.thumbXProps, "ThumbProps");
AssertType(this.props!, "ComponentProps");
AssertType(this.props, "ComponentProps");
AssertType(this, "this");
AssertType('function', "string");
AssertType(((this.props)!.thumbXProps)!.elementRef(ref), "void");
AssertType(((this.props)!.thumbXProps)!.elementRef, "ElementRef");
AssertType(((this.props)!.thumbXProps)!, "ThumbProps");
AssertType(((this.props)!.thumbXProps), "ThumbProps");
AssertType((this.props)!.thumbXProps, "ThumbProps");
AssertType((this.props)!, "ComponentProps");
AssertType((this.props), "ComponentProps");
AssertType(this.props, "ComponentProps");
AssertType(this, "this");
AssertType(ref, "union");

    };
}

