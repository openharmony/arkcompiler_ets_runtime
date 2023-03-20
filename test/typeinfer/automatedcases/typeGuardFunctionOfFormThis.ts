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

// === tests/cases/conformance/expressions/typeGuards/typeGuardFunctionOfFormThis.ts ===
declare function AssertType(value:any, type:string):void;
class RoyalGuard {
    isLeader(): this is LeadGuard {
AssertType(this instanceof LeadGuard, "boolean");
AssertType(this, "this");
AssertType(LeadGuard, "typeof LeadGuard");
        return this instanceof LeadGuard;
    }
    isFollower(): this is FollowerGuard {
AssertType(this instanceof FollowerGuard, "boolean");
AssertType(this, "this");
AssertType(FollowerGuard, "typeof FollowerGuard");
        return this instanceof FollowerGuard;
    }
}

class LeadGuard extends RoyalGuard {
    lead(): void {};
}

class FollowerGuard extends RoyalGuard {
    follow(): void {};
}

let a: RoyalGuard = new FollowerGuard();
AssertType(a, "RoyalGuard");
AssertType(new FollowerGuard(), "FollowerGuard");
AssertType(FollowerGuard, "typeof FollowerGuard");

if (a.isLeader()) {
    a.lead();
AssertType(a.lead(), "void");
AssertType(a.lead, "() => void");
}
else if (a.isFollower()) {
    a.follow();
AssertType(a.follow(), "void");
AssertType(a.follow, "() => void");
}

interface GuardInterface extends RoyalGuard {}

let b: GuardInterface;
AssertType(b, "GuardInterface");

if (b.isLeader()) {
    b.lead();
AssertType(b.lead(), "void");
AssertType(b.lead, "() => void");
}
else if (b.isFollower()) {
    b.follow();
AssertType(b.follow(), "void");
AssertType(b.follow, "() => void");
}

let holder2 = {a};
AssertType(holder2, "{ a: RoyalGuard; }");
AssertType({a}, "{ a: RoyalGuard; }");
AssertType(a, "RoyalGuard");

if (holder2.a.isLeader()) {
    holder2.a;
AssertType(holder2.a, "LeadGuard");
}
else {
    holder2.a;
AssertType(holder2.a, "RoyalGuard");
}

class ArrowGuard {
    isElite = (): this is ArrowElite => {
AssertType(this instanceof ArrowElite, "boolean");
AssertType(this, "this");
AssertType(ArrowElite, "typeof ArrowElite");
        return this instanceof ArrowElite;
    }
    isMedic = (): this is ArrowMedic => {
AssertType(this instanceof ArrowMedic, "boolean");
AssertType(this, "this");
AssertType(ArrowMedic, "typeof ArrowMedic");
        return this instanceof ArrowMedic;
    }
}

class ArrowElite extends ArrowGuard {
    defend(): void {}
}

class ArrowMedic extends ArrowGuard {
    heal(): void {}
}

let guard = new ArrowGuard();
AssertType(guard, "ArrowGuard");
AssertType(new ArrowGuard(), "ArrowGuard");
AssertType(ArrowGuard, "typeof ArrowGuard");

if (guard.isElite()) {
    guard.defend();
AssertType(guard.defend(), "void");
AssertType(guard.defend, "() => void");
}
else if (guard.isMedic()) {
    guard.heal();
AssertType(guard.heal(), "void");
AssertType(guard.heal, "() => void");
}

interface Supplies {
    spoiled: boolean;
}

interface Sundries {
    broken: boolean;
}

interface Crate<T> {
    contents: T;
    volume: number;
    isSupplies(): this is Crate<Supplies>;
    isSundries(): this is Crate<Sundries>;
}

let crate: Crate<{}>;
AssertType(crate, "Crate<{}>");

if (crate.isSundries()) {
    crate.contents.broken = true;
AssertType(crate.contents.broken = true, "boolean");
AssertType(crate.contents.broken, "boolean");
AssertType(crate.contents, "Sundries");
AssertType(true, "boolean");
}
else if (crate.isSupplies()) {
    crate.contents.spoiled = true;
AssertType(crate.contents.spoiled = true, "boolean");
AssertType(crate.contents.spoiled, "boolean");
AssertType(crate.contents, "Supplies");
AssertType(true, "boolean");
}

// Matching guards should be assignable

a.isFollower = b.isFollower;
AssertType(a.isFollower = b.isFollower, "() => this is FollowerGuard");
AssertType(a.isFollower, "() => this is FollowerGuard");
AssertType(b.isFollower, "() => this is FollowerGuard");

a.isLeader = b.isLeader;
AssertType(a.isLeader = b.isLeader, "() => this is LeadGuard");
AssertType(a.isLeader, "() => this is LeadGuard");
AssertType(b.isLeader, "() => this is LeadGuard");

class MimicGuard {
    isLeader(): this is MimicLeader { 
AssertType(this instanceof MimicLeader, "boolean");
AssertType(this, "this");
AssertType(MimicLeader, "typeof MimicLeader");
return this instanceof MimicLeader; };

    isFollower(): this is MimicFollower { 
AssertType(this instanceof MimicFollower, "boolean");
AssertType(this, "this");
AssertType(MimicFollower, "typeof MimicFollower");
return this instanceof MimicFollower; };
}

class MimicLeader extends MimicGuard {
    lead(): void {}
}

class MimicFollower extends MimicGuard {
    follow(): void {}
}

let mimic = new MimicGuard();
AssertType(mimic, "MimicGuard");
AssertType(new MimicGuard(), "MimicGuard");
AssertType(MimicGuard, "typeof MimicGuard");

a.isLeader = mimic.isLeader;
AssertType(a.isLeader = mimic.isLeader, "() => this is MimicLeader");
AssertType(a.isLeader, "() => this is LeadGuard");
AssertType(mimic.isLeader, "() => this is MimicLeader");

a.isFollower = mimic.isFollower;
AssertType(a.isFollower = mimic.isFollower, "() => this is MimicFollower");
AssertType(a.isFollower, "() => this is FollowerGuard");
AssertType(mimic.isFollower, "() => this is MimicFollower");

if (mimic.isFollower()) {
    mimic.follow();
AssertType(mimic.follow(), "void");
AssertType(mimic.follow, "() => void");

    mimic.isFollower = a.isFollower;
AssertType(mimic.isFollower = a.isFollower, "() => this is FollowerGuard");
AssertType(mimic.isFollower, "() => this is MimicFollower");
AssertType(a.isFollower, "() => this is FollowerGuard");
}


interface MimicGuardInterface {
    isLeader(): this is LeadGuard;
    isFollower(): this is FollowerGuard;
}


