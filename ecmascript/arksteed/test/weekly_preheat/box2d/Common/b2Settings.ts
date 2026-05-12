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
export let float2: number = 2.0;
export let angle180: number = 180.0;
export let half: number = 0.5;
export let float8: number = 8.0;
export let float10: number = 10.0;
export let int4: number = 4;
export let int16: number = 16;
export let float3: number = 3.0;
export let float025: number = 0.25;
export let indexTwo: number = 2;
export let indexThr: number = 3;
export let posIter: number = 20;
export let floopCount: number = 50;
export let b2MinFloat = Number.MIN_VALUE;
export let b2MaxFloat = Number.MAX_SAFE_INTEGER;
export let b2Epsilon = Number.EPSILON;
export let b2PI: number = Math.PI;
export let b2MaxManifoldPoints = 2;
export let b2MaxPolygonVertices = 8;
export let b2AaBbExtension: number = 0.1;
export let b2AaBbMultiplier: number = 2.0;
export let b2LinearSlop: number = 0.005;
export let b2AngularSlop: number = (float2 / angle180) * b2PI;
export let b2PolygonRadius: number = float2 * b2LinearSlop;
export let b2MaxSubSteps = 8;
export let b2MaxTOIContacts = 32;
export let b2VelocityThreshold: number = 1.0;
export let b2MaxLinearCorrection: number = 0.2;
export let b2MaxAngularCorrection: number = (float8 / angle180) * b2PI;
export let b2MaxTranslation: number = 2.0;
export let b2MaxTranslationSquared: number = b2MaxTranslation * b2MaxTranslation;
export let b2MaxRotation: number = half * b2PI;
export let b2MaxRotationSquared: number = b2MaxRotation * b2MaxRotation;
export let b2Baumgarte: number = 0.2;
export let b2ToiBaugarte: number = 0.75;
export let b2TimeToSleep: number = 0.5;
export let b2LinearSleepTolerance: number = 0.01;
export let b2AngularSleepTolerance: number = (float2 / angle180) * b2PI;

export let hex1: number = 0x0001;
export let hex2: number = 0x0002;
export let hex4: number = 0x0004;
export let hex8: number = 0x0008;
export let hex10: number = 0x0010;
export let hex20: number = 0x0020;
export let hex40: number = 0x0040;
