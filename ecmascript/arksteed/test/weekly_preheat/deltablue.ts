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

const STRENGTH_NUM_TWO = 2;
const STRENGTH_NUM_THREE = 3;
const STRENGTH_NUM_FOUR = 4;
const STRENGTH_NUM_FIVE = 5;
const STRENGTH_NUM_SIX = 6;
const LOOP_COUNT_OHS = 100;
const LOOP_COUNT_TWOLY = 20;
const LOOP_COUNT_OTLY = 20;
const NUM_TEN = 10;
const TIME_UNIT = 1000;
const NUM_TWO_TH = 2000;
const NUM_OOSZ = 1170;
const NUM_SEVENTEEN = 17;
const NUM_OZFZ = 1050;

class OrderedCollection<T> {
  elms: T[];

  constructor() {
    this.elms = new Array<T>();
  }

  add(elm: T): void {
    this.elms.push(elm);
  }

  at(index: number): T {
    return this.elms[index];
  }

  size(): number {
    return this.elms.length;
  }

  removeFirst(): T {
    return this.elms.pop();
  }

  remove(elm: T): void {
    let index = 0;
    let skipped = 0;
    for (let i = 0; i < this.elms.length; i++) {
      let value = this.elms[i];
      if (value !== elm) {
        this.elms[index] = value;
        index++;
      } else {
        skipped++;
      }
    }
    for (let i = 0; i < skipped; i++) {
      this.elms.pop();
    }
  }
}

/**
 * Strengths are used to measure the relative importance of constraints.
 * New strengths may be inserted in the strength hierarchy without
 * disrupting current constraints.  Strengths cannot be created outside
 * this class, so pointer comparison can be used for value comparison.
 */
class Strength {
  strengthValue: number;
  name: string;

  static strRequired = new Strength(0, 'required');
  static strongPreferred = new Strength(1, 'strongPreferred');
  static preferred = new Strength(STRENGTH_NUM_TWO, 'preferred');
  static strongDefault = new Strength(STRENGTH_NUM_THREE, 'strongDefault');
  static strNormal = new Strength(STRENGTH_NUM_FOUR, 'normal');
  static weakDefault = new Strength(STRENGTH_NUM_FIVE, 'weakDefault');
  static strWeakest = new Strength(STRENGTH_NUM_SIX, 'weakest');

  constructor(strengthValue: number, name: string) {
    this.strengthValue = strengthValue;
    this.name = name;
  }

  static stronger(s1: Strength, s2: Strength): boolean {
    return s1.strengthValue < s2.strengthValue;
  }

  static weaker(s1: Strength, s2: Strength): boolean {
    return s1.strengthValue > s2.strengthValue;
  }

  static weakestOf(s1: Strength, s2: Strength): Strength {
    return Strength.weaker(s1, s2) ? s1 : s2;
  }

  static strongest(s1: Strength, s2: Strength): Strength {
    return Strength.stronger(s1, s2) ? s1 : s2;
  }

  nextWeaker(): Strength | null {
    switch (this.strengthValue) {
      case 0:
        return Strength.strWeakest; //strWeakest
      case 1:
        return Strength.weakDefault; //weakDefault
      case STRENGTH_NUM_TWO:
        return Strength.strNormal; //strNormal
      case STRENGTH_NUM_THREE:
        return Strength.strongDefault; //strongDefault
      case STRENGTH_NUM_FOUR:
        return Strength.preferred; //preferred
      case STRENGTH_NUM_FIVE:
        return Strength.strRequired; //strRequired
      default:
        return null;
    }
  }
}

/**
 ** An abstract class representing a system-maintainable relationship
 ** (or "constraint") between a set of variables. A constraint supplies
 ** a strength instance variable; concrete subclasses provide a means
 ** of storing the constrained variables and other information required
 ** to represent a constraint.
 **/

class Constraint {
  strength: Strength;
  constructor(strength: Strength) {
    this.strength = strength;
  }
  /**
   ** Activate this constraint and attempt to satisfy it.
   **/
  addConstraint(): void {
    this.addToGraph();
    planner.incrementalAdd(this);
  }

  /**
   ** Attempt to find a way to enforce this constraint. If successful,
   ** record the solution, perhaps modifying the current dataflow
   ** graph. Answer the constraint that this constraint overrides, if
   ** there is one, or nil, if there isn't.
   ** Assume: I am not already satisfied.
   **/
  satisfy(mark: number): Constraint | null {
    this.chooseMethod(mark);
    if (!this.isSatisfied()) {
      if (this.strength !== Strength.strRequired) {
        ////debugMsg("Could not satisfy a required constraint!")
      }
      return null;
    }
    this.markInputs(mark);
    let out = this.output();

    let overridden = out.determinedBy;
    if (overridden !== null) {
      overridden.markUnsatisfied();
    }
    out.determinedBy = this;
    if (!planner.addPropagate(this, mark)) {
      ////debugMsg("Cycle encountered")
    }
    out.mark = mark;
    return overridden;
  }
  destroyConstraint(): void {
    if (this.isSatisfied()) {
      planner.incrementalRemove(this);
    } else {
      this.removeFromGraph();
    }
  }

  /**
   * Normal constraints are not input constraints.  An input constraint
   * is one that depends on external state, such as the mouse, the
   * keybord, a clock, or some arbitraty piece of imperative code.
   */
  isInput(): boolean {
    return false;
  }
  markUnsatisfied(): void {}
  recalculate(): void {}
  isSatisfied(): boolean {
    return true;
  }
  removeFromGraph(): void {}
  inputsKnown(mark: number): boolean {
    return true;
  }
  execute(): void {}
  addToGraph(): void {}
  chooseMethod(mark: number): void {}
  markInputs(mark: number): void {}
  output(): Variable {
    return null;
  }
}

class UnaryConstraint extends Constraint {
  myOutput: Variable | null;
  satisfied = false;

  /**
   * Abstract superclass for constraints having a single possible output
   * variable.
   */
  constructor(v: Variable, strength: Strength) {
    super(strength);
    this.myOutput = v;
    this.satisfied = false;
    this.addConstraint();
  }

  /**
   * Adds this constraint to the constraint graph
   */
  addToGraph(): void {
    this.myOutput.addConstraint(this);
    this.satisfied = false;
  }

  /**
   * Decides if this constraint can be satisfied and records that
   * decision.
   */
  chooseMethod(mark: number): void {
    this.satisfied = this.myOutput.mark !== mark && Strength.stronger(this.strength, this.myOutput.walkStrength);
  }

  /**
   * Returns true if this constraint is satisfied in the current solution.
   */
  isSatisfied(): boolean {
    return this.satisfied;
  }
  markInputs(mark: number): void {}

  /**
   * Returns the current output variable.
   */
  output(): Variable {
    return this.myOutput;
  }

  /**
   * Calculate the walkabout strength, the stay flag, and, if it is
   * 'stay', the value for the current output of this constraint. Assume
   * this constraint is satisfied.
   */
  recalculate(): void {
    this.myOutput.walkStrength = this.strength;
    this.myOutput.stay = !this.isInput();
    if (this.myOutput.stay) {
      this.execute();
    }
  }

  /**
   * Records that this constraint is unsatisfied
   */
  markUnsatisfied(): void {
    this.satisfied = false;
  }
  inputsKnown(mark: number): boolean {
    return true;
  }
  removeFromGraph(): void {
    if (this.myOutput !== null) {
      this.myOutput.removeConstraint(this);
    }
    this.satisfied = false;
  }
}

/**
 * Variables that should, with some level of preference, stay the same.
 * Planners may exploit the fact that instances, if satisfied, will not
 * change their output during plan execution.  This is called "stay
 * optimization".
 */
class StayConstraint extends UnaryConstraint {
  constructor(v: Variable, str: Strength) {
    super(v, str);
  }

  execute(): void {}
}

/**
 * A unary input constraint used to mark a variable that the client
 * wishes to change.
 */
class EditConstraint extends UnaryConstraint {
  constructor(v: Variable, str: Strength) {
    super(v, str);
  }

  /**
   * Edits indicate that a variable is to be changed by imperative code.
   */
  isInput(): boolean {
    return true;
  }

  execute(): void {}
}

enum Direction {
  NONE = 0,
  FORWARD = 1,
  BACKWARD = -1
}

/**
 * Abstract superclass for constraints having two possible output
 * variables.
 */
class BinaryConstraint extends Constraint {
  v1: Variable | null;
  v2: Variable | null;
  direction: Direction = Direction.NONE;

  constructor(var1: Variable, var2: Variable, strength: Strength) {
    super(strength);
    this.v1 = var1;
    this.v2 = var2;
    this.addConstraint();
  }

  /**
   * Decides if this constraint can be satisfied and which way it
   * should flow based on the relative strength of the variables related,
   * and record that decision.
   */
  chooseMethod(mark: number): void {
    if (this.v1.mark === mark) {
      // relative v1.mark
      this.direction = this.v2.mark !== mark && Strength.stronger(this.strength, this.v2.walkStrength) ? Direction.FORWARD : Direction.NONE;
    }
    if (this.v2.mark === mark) {
      // relative v2.mark
      this.direction = this.v1.mark !== mark && Strength.stronger(this.strength, this.v1.walkStrength) ? Direction.BACKWARD : Direction.NONE;
    }
    if (Strength.weaker(this.v1.walkStrength, this.v2.walkStrength)) {
      // weaker v1 and v2
      this.direction = Strength.stronger(this.strength, this.v1.walkStrength) ? Direction.BACKWARD : Direction.NONE;
    } else {
      // stronger v1 and v2
      this.direction = Strength.stronger(this.strength, this.v2.walkStrength) ? Direction.FORWARD : Direction.BACKWARD;
    }
  }

  /***
   ** Add this constraint to the constraint graph
   **/
  addToGraph(): void {
    this.v1.addConstraint(this);
    this.v2.addConstraint(this);
    this.direction = Direction.NONE;
  }

  /**
   * Answer true if this constraint is satisfied in the current solution.
   */
  isSatisfied(): boolean {
    return this.direction !== Direction.NONE;
  }

  /**
   * Mark the input variable with the given mark.
   */
  markInputs(mark: number): void {
    this.input().mark = mark;
  }

  /**
   * Returns the current output variable
   */
  input(): Variable {
    return this.direction === Direction.FORWARD ? this.v1 : this.v2;
  }

  /**
   * Returns the current input variable
   */
  output(): Variable {
    return this.direction === Direction.FORWARD ? this.v2 : this.v1;
  }

  /**
   * Calculate the walkabout strength, the stay flag, and, if it is
   * 'stay', the value for the current output of this
   * constraint. Assume this constraint is satisfied.
   */
  recalculate(): void {
    let ihn = this.input();
    let out = this.output();
    out.walkStrength = Strength.weakestOf(this.strength, ihn.walkStrength);
    out.stay = ihn.stay;
    if (out.stay) {
      this.execute();
    }
  }

  /**
   * Record the fact that this constraint is unsatisfied.
   */
  markUnsatisfied(): void {
    this.direction = Direction.NONE;
  }

  inputsKnown(mark: number): boolean {
    let i = this.input();
    return i.mark === mark || i.stay || i.determinedBy === null;
  }

  removeFromGraph(): void {
    if (this.v1 !== null) {
      this.v1.removeConstraint(this);
    }
    if (this.v2 !== null) {
      this.v2.removeConstraint(this);
    }
    this.direction = Direction.NONE;
  }
}

/**
 * Relates two variables by the linear scaling relationship: "v2 =
 * (v1 * scale) + offset". Either v1 or v2 may be changed to maintain
 * this relationship but the scale factor and offset are considered
 * read-only.
 */
class ScaleConstraint extends BinaryConstraint {
  scale: Variable;
  offset: Variable;
  direction: Direction = Direction.NONE;

  constructor(src: Variable, scale: Variable, offset: Variable, des: Variable, strength: Strength) {
    super(src, des, strength);
    this.scale = scale;
    this.offset = offset;
    this.realAddConstraint();
  }

  addConstraint(): void {}
  realAddConstraint(): void {
    super.addConstraint();
  }

  addToGraph(): void {
    super.addToGraph();
    this.scale.addConstraint(this);
    this.offset.addConstraint(this);
  }
  removeFromGraph(): void {
    super.removeFromGraph();
    if (this.scale !== null) {
      this.scale.removeConstraint(this);
    }
    if (this.offset !== null) {
      this.offset.removeConstraint(this);
    }
  }
  markInputs(mark: number): void {
    super.markInputs(mark);
    this.scale.mark = mark;
    this.offset.mark = mark;
  }

  /**
   * Enforce this constraint. Assume that it is satisfied.
   */
  execute(): void {
    if (this.direction === Direction.FORWARD) {
      // Direction.FORWARD
      this.v2.value = this.v1.value * this.scale.value + this.offset.value;
    } else {
      this.v1.value = (this.v2.value - this.offset.value) / this.scale.value;
    }
  }
  /**
   ** Calculate the walkabout strength, the stay flag, and, if it is
   ** 'stay', the value for the current output of this constraint. Assume
   ** this constraint is satisfied.
   **/
  recalculate(): void {
    let ihn = this.input();
    let out = this.output();
    out.walkStrength = Strength.weakestOf(this.strength, ihn.walkStrength);
    out.stay = ihn.stay && this.scale.stay && this.offset.stay;
    if (out.stay) {
      this.execute();
    }
  }
}

/**
 * Constrains two variables to have the same value.
 */
class EqualityConstraint extends BinaryConstraint {
  constructor(var1: Variable, var2: Variable, strength: Strength) {
    super(var1, var2, strength);
  }

  execute(): void {
    this.output().value = this.input().value;
  }
}

/**
 ** A constrained variable. In addition to its value, it maintain the
 ** structure of the constraint graph, the current dataflow graph, and
 ** various parameters of interest to the DeltaBlue incremental
 ** constraint solver.
 **/
class Variable {
  name: string;
  constraints: OrderedCollection<Constraint>;
  value: number;
  determinedBy: null | Constraint;
  mark: number;
  walkStrength: Strength;
  stay: boolean;

  constructor(name: string, initialValue?: number) {
    this.value = initialValue || 0;
    this.constraints = new OrderedCollection();
    this.mark = 0;
    this.walkStrength = Strength.strWeakest;
    this.name = name;
    this.determinedBy = null;
    this.stay = true;
  }

  /**
   ** Add the given constraint to the set of all constraints that refer
   ** this variable.
   **/
  addConstraint(c: Constraint): void {
    this.constraints.add(c);
  }

  /**
   * Removes all traces of c from this variable.
   */
  removeConstraint(c: Constraint): void {
    this.constraints.remove(c);
    if (this.determinedBy === c) {
      this.determinedBy = null;
    }
  }
}

let planner: Planner;

class Planner {
  currentMark = 0;

  /**
   ** Attempt to satisfy the given constraint and, if successful,
   ** incrementally update the dataflow graph.  Details: If satifying
   ** the constraint is successful, it may override a weaker constraint
   ** on its output. The algorithm attempts to resatisfy that
   ** constraint using some other method. This process is repeated
   ** until either a) it reaches a variable that was not previously
   ** determined by any constraint or b) it reaches a constraint that
   ** is too weak to be satisfied using any of its methods. The
   ** variables of constraints that have been processed are marked with
   ** a unique mark value so that we know where we've been. This allows
   ** the algorithm to avoid getting into an infinite loop even if the
   ** constraint graph has an inadvertent cycle.
   **/
  incrementalAdd(c: Constraint): void {
    let mark = this.newMark();
    let overridden = c.satisfy(mark);
    while (overridden !== null) {
      overridden = overridden.satisfy(mark);
    }
  }

  /**
   ** Entry point for retracting a constraint. Remove the given
   ** constraint and incrementally update the dataflow graph.
   ** Details: Retracting the given constraint may allow some currently
   ** unsatisfiable downstream constraint to be satisfied. We therefore collect
   ** a list of unsatisfied downstream constraints and attempt to
   ** satisfy each one in turn. This list is traversed by constraint
   ** strength, strongest first, as a heuristic for avoiding
   ** unnecessarily adding and then overriding weak constraints.
   ** Assume: c is satisfied.
   **/
  incrementalRemove(c: Constraint): void {
    let out = c.output();
    c.markUnsatisfied();
    c.removeFromGraph();
    let unsatisfied = this.removePropagateFrom(out);
    let stren = Strength.strRequired;
    do {
      for (let i = 0; i < unsatisfied.size(); i++) {
        let u = unsatisfied.at(i);
        if (u.strength === stren) {
          this.incrementalAdd(u);
        }
      }
      stren = stren.nextWeaker();
    } while (stren !== Strength.strWeakest);
  }

  /**
   ** Select a previously unused mark value.
   **/
  newMark(): number {
    return ++this.currentMark;
  }

  /**
   ** Extract a plan for resatisfaction starting from the given source
   ** constraints, usually a set of input constraints. This method
   ** assumes that stay optimization is desired; the plan will contain
   ** only constraints whose output variables are not stay. Constraints
   ** that do no computation, such as stay and edit constraints, are
   ** not included in the plan.
   ** Details: The outputs of a constraint are marked when it is added
   ** to the plan under construction. A constraint may be appended to
   ** the plan when all its input variables are known. A variable is
   ** known if either a) the variable is marked (indicating that has
   ** been computed by a constraint appearing earlier in the plan), b)
   ** the variable is 'stay' (i.e. it is a constant at plan execution
   ** time), or c) the variable is not determined by any
   ** constraint. The last provision is for past states of history
   ** variables, which are not stay but which are also not computed by
   ** any constraint.
   ** Assume: sources are all satisfied.
   **/
  makePlan(sources: OrderedCollection<Constraint>): Plan {
    let mark = this.newMark();
    let plan = new Plan();
    let todo = sources;
    while (todo.size() > 0) {
      let constraint = todo.removeFirst();
      if (constraint.output().mark !== mark && constraint.inputsKnown(mark)) {
        plan.addConstraint(constraint);
        constraint.output().mark = mark;
        this.addConstraintsConsumingTo(constraint.output(), todo);
      }
    }
    return plan;
  }

  /**
   ** Extract a plan for resatisfying starting from the output of the
   ** given constraints, usually a set of input constraints.
   **/
  extractPlanFromConstraints(constraints: OrderedCollection<Constraint>): Plan {
    let sources = new OrderedCollection<Constraint>();
    for (let i = 0; i < constraints.size(); i++) {
      let constraint = constraints.at(i);
      if (constraint.isInput() && constraint.isSatisfied()) {
        sources.add(constraint);
      }
    }
    return this.makePlan(sources);
  }

  /**
   ** Recompute the walkabout strengths and stay flags of all variables
   ** downstream of the given constraint and recompute the actual
   ** values of all variables whose stay flag is true. If a cycle is
   ** detected, remove the given constraint and answer
   ** false. Otherwise, answer true.
   ** Details: Cycles are detected when a marked variable is
   ** encountered downstream of the given constraint. The sender is
   ** assumed to have marked the inputs of the given constraint with
   ** the given mark. Thus, encountering a marked node downstream of
   ** the output constraint means that there is a path from the
   ** constraint's output to one of its inputs.
   **/
  addPropagate(c: Constraint, mark: number): boolean {
    let todo = new OrderedCollection<Constraint>();
    todo.add(c);
    while (todo.size() > 0) {
      let dered = todo.removeFirst();
      if (dered.output().mark === mark) {
        this.incrementalRemove(c);
        return false;
      }
      dered.recalculate();
      this.addConstraintsConsumingTo(dered.output(), todo);
    }
    return true;
  }
  addConstraintsConsumingTo(v: Variable, coll: OrderedCollection<Constraint>): void {
    let determining = v.determinedBy;
    let cc = v.constraints;
    for (let i = 0; i < cc.size(); i++) {
      let c = cc.at(i);
      if (c !== determining && c.isSatisfied()) {
        coll.add(c);
      }
    }
  }

  /**
   * Update the walkabout strengths and stay flags of all variables
   * downstream of the given constraint. Answer a collection of
   * unsatisfied constraints sorted in order of decreasing strength.
   */
  removePropagateFrom(out: Variable): OrderedCollection<Constraint> {
    out.determinedBy = null;
    out.walkStrength = Strength.strWeakest;
    out.stay = true;
    let unsatisfied = new OrderedCollection<Constraint>();
    let todo = new OrderedCollection<Variable>();
    todo.add(out);
    while (todo.size() > 0) {
      let v = todo.removeFirst();
      for (let i = 0; i < v.constraints.size(); i++) {
        let c = v.constraints.at(i);
        if (!c.isSatisfied()) {
          unsatisfied.add(c);
        }
      }
      let determining = v.determinedBy;
      for (let i = 0; i < v.constraints.size(); i++) {
        let next = v.constraints.at(i);
        if (next !== determining && next.isSatisfied()) {
          next.recalculate();
          todo.add(next.output());
        }
      }
    }
    return unsatisfied;
  }
}

/**
 * A Plan is an ordered list of constraints to be executed in sequence
 * to resatisfy all currently satisfiable constraints in the face of
 * one or more changing inputs.
 */
class Plan {
  v: OrderedCollection<Constraint> = new OrderedCollection<Constraint>();

  addConstraint(c: Constraint): void {
    this.v.add(c);
  }

  size(): number {
    return this.v.size();
  }

  constraintAt(index: number): void {
    return this.v.at(index);
  }

  execute(): void {
    for (let i = 0; i < this.size(); i++) {
      let c = this.constraintAt(i);
      c.execute();
    }
  }
}

/**
 ** This is the standard DeltaBlue benchmark. A long chain of equality
 ** constraints is constructed with a stay constraint on one end. An
 ** edit constraint is then added to the opposite end and the time is
 ** measured for adding and removing this constraint, and extracting
 ** and executing a constraint satisfaction plan. There are two cases.
 ** In case 1, the added constraint is stronger than the stay
 ** constraint and values must propagate down the entire length of the
 ** chain. In case 2, the added constraint is weaker than the stay
 ** constraint so it cannot be accomodated. The cost in this case is,
 ** of course, very low. Typical situations lie somewhere between these
 ** two extremes.
 **/
function chainTest(n: number): void {
  planner = new Planner();
  let prev: Variable = null;
  let first: Variable = null;
  let last: Variable = null;
  for (let i = 0; i <= n; i++) {
    let name = 'v' + i;
    let v = new Variable(name, 0);
    if (prev !== null) {
      new EqualityConstraint(prev, v, Strength.strRequired);
    }
    if (i === 0) {
      first = v;
    }
    if (i === n) {
      last = v;
    }
    prev = v;
  }
  new StayConstraint(last, Strength.strongDefault);
  let edit = new EditConstraint(first, Strength.preferred);
  let edits = new OrderedCollection<Constraint>();
  edits.add(edit);
  let plan = planner.extractPlanFromConstraints(edits);
  for (let i = 0; i < LOOP_COUNT_OHS; i++) {
    if (first) {
      first.value = i;
    }
    ////debugMsg(first.value + 'is first.value')
    plan.execute();
    ////debugMsg(last.value + 'is last.value')
    if (last?.value !== i) {
      ////debugMsg("Chain test failed.")
    }
  }
}

function projectionTest(n: number): void {
  planner = new Planner();
  let scale = new Variable('scale', NUM_TEN);
  let offset = new Variable('offset', TIME_UNIT);
  let src: Variable = null;
  let dst: Variable = null;
  let des = new OrderedCollection<Variable>();
  for (let i = 0; i < n; i++) {
    src = new Variable('src' + i, i);
    dst = new Variable('dst' + i, i);
    des.add(dst);
    new StayConstraint(src, Strength.strNormal);
    new ScaleConstraint(src, scale, offset, dst, Strength.strRequired);
  }

  ////debugMsg("src original value is " + src.value)
  ////debugMsg("dst original value is " + dst.value)
  change(src, NUM_SEVENTEEN);
  ////debugMsg("src new value is " + src.value)
  ////debugMsg("dst new value is " + dst.value)

  if (dst.value !== NUM_OOSZ) {
    ////debugMsg("Projection 1 failed")
  }
  ////debugMsg("src original value is " + src.value)
  ////debugMsg("dst original value is" + dst.value)
  change(dst, NUM_OZFZ);
  ////debugMsg("src new value is " + src.value)
  ////debugMsg("dst new value is " + dst.value)
  if (src.value !== STRENGTH_NUM_FIVE) {
    ////debugMsg("Projection 2 failed")
  }
  change(scale, STRENGTH_NUM_FIVE);
  for (let i = 0; i < n - 1; i++) {
    if (des.at(i).value !== i * STRENGTH_NUM_FIVE + TIME_UNIT) {
      ////debugMsg("Projection 3 failed")
    }
  }
  change(offset, NUM_TWO_TH);
  for (let i = 0; i < n - 1; i++) {
    if (des.at(i).value !== i * STRENGTH_NUM_FIVE + NUM_TWO_TH) {
      ////debugMsg("Projection 4 failed")
    }
  }
}

function change(v: Variable, newValue: number): void {
  let edit = new EditConstraint(v, Strength.preferred);
  let edits = new OrderedCollection<Constraint>();
  edits.add(edit);
  let plan = planner.extractPlanFromConstraints(edits);
  for (let i = 0; i < NUM_TEN; i++) {
    v.value = newValue;
    plan.execute();
  }
  edit.destroyConstraint();
}

function deltaBlue(): void {
  chainTest(LOOP_COUNT_OHS);
  projectionTest(LOOP_COUNT_OHS);
}

declare interface ArkTools {
  timeInUs(args: number): number;
}

/*
 * @State
 * @Tags Jetstream2
 */
class Benchmark {
  runIteration(): void {
    for (let i = 0; i < LOOP_COUNT_TWOLY; ++i) {
      deltaBlue();
    }
  }

  /*
   * @Benchmark
   */
  runIterationTime(): void {
    let start = ArkTools.timeInUs() / TIME_UNIT;
    for (let i = 0; i < LOOP_COUNT_OTLY; ++i) {
      this.runIteration();
    }
    let end = ArkTools.timeInUs() / TIME_UNIT;
    print('deltablue: ms = ' + (end - start));
  }
}

let debug: boolean = false;
function debugMsg(msg: string): void {
  if (debug) {
    print(msg);
  }
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  new Benchmark().runIterationTime();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

new Benchmark().runIterationTime();
