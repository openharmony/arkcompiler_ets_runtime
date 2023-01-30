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

// === tests/cases/compiler/circularlySimplifyingConditionalTypesNoCrash.ts ===
declare function AssertType(value:any, type:string):void;
type Omit<T, K extends keyof T> = Pick<T, Exclude<keyof T, K>>;

type Shared< // Circularly self constraining type, defered thanks to mapping
    InjectedProps,
    DecorationTargetProps extends Shared<InjectedProps, DecorationTargetProps>
    > = {
        [P in Extract<keyof InjectedProps, keyof DecorationTargetProps>]: InjectedProps[P] extends DecorationTargetProps[P] ? DecorationTargetProps[P] : never;
    };

interface ComponentClass<P> {
    defaultProps?: Partial<P>; // Inference target is also mapped _and_ optional
}

interface InferableComponentEnhancerWithProps<TInjectedProps, TNeedsProps> {
    <P extends Shared<TInjectedProps, P>>(
        component: ComponentClass<P>
    ): ComponentClass<Omit<P, keyof Shared<TInjectedProps, P>> & TNeedsProps> & { WrappedComponent: ComponentClass<P> }
} // Then intersected with and indexed via Omit and &

interface Connect { // Then strictly compared with another signature in its context
    <TStateProps, TOwnProps>(
        mapStateToProps: unknown,
    ): InferableComponentEnhancerWithProps<TStateProps, TOwnProps>;

    <TDispatchProps, TOwnProps>(
        mapStateToProps: null | undefined,
        mapDispatchToProps: unknown,
        mergeProps: null | undefined,
        options: unknown
    ): InferableComponentEnhancerWithProps<TDispatchProps, TOwnProps>;
}

declare let connect: Connect;
AssertType(connect, "Connect");

const myStoreConnect: Connect = function(
AssertType(myStoreConnect, "Connect");
AssertType(function(    mapStateToProps?: any,    mapDispatchToProps?: any,    mergeProps?: any,    options: unknown = {},) {    return connect(        mapStateToProps,        mapDispatchToProps,        mergeProps,        options,    );}, "<TStateProps, TOwnProps>(?any, ?any, ?any, ?unknown) => InferableComponentEnhancerWithProps<TStateProps, Omit<P, Extract<keyof TStateProps, keyof P>> & TOwnProps>");

    mapStateToProps?: any,
AssertType(mapStateToProps, "any");

    mapDispatchToProps?: any,
AssertType(mapDispatchToProps, "any");

    mergeProps?: any,
AssertType(mergeProps, "any");

    options: unknown = {},
AssertType(options, "unknown");
AssertType({}, "{}");

) {
AssertType(connect(        mapStateToProps,        mapDispatchToProps,        mergeProps,        options,    ), "InferableComponentEnhancerWithProps<TStateProps, Omit<P, Extract<keyof TStateProps, keyof P>> & TOwnProps>");
AssertType(connect, "Connect");
    return connect(

        mapStateToProps,
AssertType(mapStateToProps, "any");

        mapDispatchToProps,
AssertType(mapDispatchToProps, "any");

        mergeProps,
AssertType(mergeProps, "any");

        options,
AssertType(options, "unknown");

    );
};

export {};


