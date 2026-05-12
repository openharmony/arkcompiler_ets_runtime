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

import { File } from './file';
import { registerPattern, fprPattern } from './registers';
import { instructionSet } from './instructions';
import {
  Node,
  StructOffset,
  Immediate,
  Sizeof,
  AddImmediates,
  SubImmediates,
  MulImmediates,
  NegImmediate,
  OrImmediates,
  AndImmediates,
  XorImmediates,
  BitnotImmediate,
  RegisterId,
  FpRegisterId,
  Variable,
  Address,
  BaseIndex,
  AbsoluteAddress,
  Instruction,
  Error,
  ConstExpr,
  ConstDecl,
  Label,
  LocalLabel,
  LabelReference,
  LocalLabelReference,
  Sequence,
  True,
  False,
  Setting,
  And,
  Or,
  Not,
  StringLiteral,
  IfThenElse,
  Macro,
  MacroCall,
  SourceFile,
  CodeOrigin
} from './ast';

function print(msg: string): void {
  console.log(msg);
}

let debug: boolean = false;
function debugLog(msg: string): void {
  if (debug) {
    print(msg);
  }
}

let globalAnnotation: string = 'global';
let localAnnotation: string = 'local';

/*
 * Create IncludeFile File Class Splice
 */
export class IncludeFile extends Node {
  includeDirs: string = '';
  fileName: string = '';

  constructor(moduleName: string, defaultDir: string) {
    super(null);
    this.fileName = moduleName + '.asm';
  }

  toString(): string {
    return this.fileName;
  }
}

/*
 * Define Token Class
 */
export class Token {
  codeOrigin: CodeOrigin;
  string: string;
  constructor(codeOrigi: CodeOrigin, string: string) {
    this.codeOrigin = codeOrigi;
    this.string = string;
  }

  isEqualTo(other: Token | string): boolean {
    if (other instanceof Token) {
      return this.string === other.string;
    } else {
      return this.string === other;
    }
  }

  isNotEqualTo(other: Token | string): boolean {
    return !this.isEqualTo(other);
  }

  toString(): string {
    return '' + this.string + '\\at ' + this.codeOrigin.toString();
  }

  parseError(comment: string | null): void {
    print('Parse error:' + comment);
  }
}

/*
 * Define Annotation class
 */
export class Annotation {
  codeOrigin: CodeOrigin;
  string: string = '';
  type: string;

  constructor(codeOrigi: CodeOrigin, typ: string, strin: string) {
    this.codeOrigin = codeOrigi;
    this.type = typ;
    this.string = strin;
  }

  isEqualTo(other: Token | string): boolean {
    if (other instanceof Token) {
      return this.string === other.string;
    } else {
      return this.string === other;
    }
  }

  isNotEqualTo(other: Annotation | string | Token): boolean {
    return !this.isEqualTo(other);
  }

  toString(): string {
    return '' + this.string + 'at' + this.codeOrigin.toString();
  }

  parseError(comment: string | null): void {
    print('Annotation error:' + comment);
  }
}

/*
 * Regex gets a data array containing a character
 */
function scanRegExp(source: string, regexp: string): string[] | null {
  return source.match(regexp);
}

/*
 * Returns a Token Array
 */
function lex(strs: string, file: SourceFile): Array<Token | Annotation> {
  // The lexer. Takes a string and returns an array of tokens.
  let result: Array<Token | Annotation> = [];
  let lineNumber: number = 1;
  let whitespaceFound: boolean = false;
  let str = strs;
  while (str.length) {
    let tokenMatch: string[] | null;
    let annotation: string | null = null;
    let annotationType: string = '';

    if ((tokenMatch = scanRegExp(str, '^#([^\n]*)'))) {
      // comment, ignore
      tokenMatch = tokenMatch;
    } else if ((tokenMatch = scanRegExp(str, '^// ?([^\n]*)'))) {
      // annotation
      tokenMatch = tokenMatch;
      annotation = tokenMatch[0];
      annotationType = whitespaceFound ? localAnnotation : globalAnnotation;
    } else if ((tokenMatch = scanRegExp(str, '^\n'))) {
      tokenMatch = tokenMatch;
      if (annotation) {
        result.push(new Annotation(new CodeOrigin(file, lineNumber), annotationType, annotation));
        annotation = null;
      }
      result.push(new Token(new CodeOrigin(file, lineNumber), tokenMatch[0]));
      lineNumber += 1;
    } else if ((tokenMatch = scanRegExp(str, '^([ \t]+)'))) {
      // whitespace, ignore
      whitespaceFound = true;
      str = str.slice(tokenMatch[0].length);
      continue;
    } else {
      let tto = tokenMatchByScanRegExp(tokenMatch, result, file, lineNumber, str);
      tokenMatch = tto[0];
      result = tto[1];
    }

    whitespaceFound = false;
    str = str.slice(tokenMatch![0].length);
  }

  // if (debug) {
  //   for (let i = 0; i < result.length; i++) {
  // debugLog(i + ' line in result: ' + result[i].toString());
  //   }
  // }

  return result;
}

function tokenMatchByScanRegExp(
  tokenMatch: string[] | null,
  result: Array<Token | Annotation>,
  file: SourceFile,
  lineNumber: number,
  str: string
): [string[] | null, Array<Token | Annotation>] {
  if ((tokenMatch = scanRegExp(str, '^[a-zA-Z]([a-zA-Z0-9_.]*)'))) {
    tokenMatch = tokenMatch;
    result.push(new Token(new CodeOrigin(file, lineNumber), tokenMatch[0]));
  } else if ((tokenMatch = scanRegExp(str, '^\\.([a-zA-Z0-9_]*)'))) {
    tokenMatch = tokenMatch;
    result.push(new Token(new CodeOrigin(file, lineNumber), tokenMatch[0]));
  } else if ((tokenMatch = scanRegExp(str, '^_([a-zA-Z0-9_]*)'))) {
    tokenMatch = tokenMatch;
    result.push(new Token(new CodeOrigin(file, lineNumber), tokenMatch[0]));
  } else if ((tokenMatch = scanRegExp(str, '^0x([0-9a-fA-F]+)'))) {
    tokenMatch = tokenMatch;
    result.push(new Token(new CodeOrigin(file, lineNumber), Number.parseInt(tokenMatch[1], 16).toString()));
  } else if ((tokenMatch = scanRegExp(str, '^0([0-7]+)'))) {
    tokenMatch = tokenMatch;
    result.push(new Token(new CodeOrigin(file, lineNumber), Number.parseInt(tokenMatch[1], 8).toString()));
  } else if ((tokenMatch = scanRegExp(str, '^([0-9]+)'))) {
    tokenMatch = tokenMatch;
    result.push(new Token(new CodeOrigin(file, lineNumber), tokenMatch[0]));
  } else if ((tokenMatch = scanRegExp(str, '^::'))) {
    // Match with double colons:: Text line at the beginning
    tokenMatch = tokenMatch;
    result.push(new Token(new CodeOrigin(file, lineNumber), tokenMatch[0]));
  } else if ((tokenMatch = scanRegExp(str, '^[:,\\(\\)\\[\\]=\\+\\-~\\|&^*]'))) {
    // ^[:,\(\)\[\]=\+\-~\|&^*] Matches strings starting with colon, comma, parenthesis,
    // square brackets, equals, plus, minus, tilde, pipe, and, or characters, or asterisks
    tokenMatch = tokenMatch;
    result.push(new Token(new CodeOrigin(file, lineNumber), tokenMatch[0]));
  } else if ((tokenMatch = scanRegExp(str, '^".*"'))) {
    // Matches strings enclosed in double quotes
    tokenMatch = tokenMatch;
    result.push(new Token(new CodeOrigin(file, lineNumber), tokenMatch[0]));
  } else {
    print(`Lexer error at ${new CodeOrigin(file, lineNumber)}`);
  }

  return [tokenMatch, result];
}

// Token identification.
function isRegister(token: Token): boolean {
  return registerPattern.test(token.string);
}

function isInstruction(token: Token): boolean {
  return instructionSet.has(token.string);
}

function isKeyword(token: Token): boolean {
  return (
    RegExp('^((true)|(false)|(if)|(then)|(else)|(elsif)|(end)|(and)|' + '(or)|(not)|(global)|(macro)|(const)|(constexpr)|(sizeof)|(error)|(include))$').test(
      token.string
    ) ||
    isRegister(token) ||
    isInstruction(token)
  );
}

function isIdentifier(token: Token): boolean {
  return RegExp('^[a-zA-Z]([a-zA-Z0-9_.]*)$').test(token.string) && !isKeyword(token);
}

function isLabel(token: Token | string): boolean {
  let tokenString: string;
  if (token instanceof Token) {
    tokenString = token.string;
  } else {
    tokenString = token;
  }
  // Matches regular expressions that start with an underscore followed by letters, numbers, or underscores
  return RegExp('^_([a-zA-Z0-9_]*)$').test(tokenString);
}

function isLocalLabel(token: string | Token): boolean {
  let tokenString: string;
  if (token instanceof Token) {
    tokenString = token.string;
  } else {
    tokenString = token;
  }
  return RegExp('^\\.([a-zA-Z0-9_]*)$').test(tokenString);
}

function isVariable(token: Token): boolean {
  return isIdentifier(token) || isRegister(token);
}

function isInteger(token: Token): boolean {
  return RegExp('^[0-9]').test(token.string);
}

function isString(token: Token): boolean {
  return RegExp('^".*"').test(token.string);
}

// The parser. Takes an array of tokens and returns an AST. Methods
// other than parse(tokens) are not for public consumption.
export class Parser {
  tokens = Array<Token>();
  idx: number = 0;
  annotation: string | null;

  constructor(data: string, fileName: SourceFile) {
    this.tokens = lex(data, fileName) as Array<Token>;
    this.idx = 0;
    this.annotation = null;
  }

  parseError(comment: string | null): void {
    if (this.tokens[this.idx] !== undefined) {
      this.tokens[this.idx].parseError(comment);
    } else {
      if (comment != null) {
        print('Parse error at end of file');
      } else {
        print('Parse error at end of file: ' + comment);
      }
    }
  }

  consume(regexp: string): void {
    if (regexp) {
      if (!RegExp(regexp).test(this.tokens[this.idx].string)) {
        this.parseError(null);
      }
    } else if (this.idx !== this.tokens.length) {
      this.parseError(null);
    }
    this.idx += 1;
  }

  skipNewLine(): void {
    while (this.tokens[this.idx].isEqualTo('\n')) {
      this.idx += 1;
    }
  }

  parsePredicateAtom(): Node | null {
    if (this.tokens[this.idx].isEqualTo('not')) {
      let codeOrigin = this.tokens[this.idx].codeOrigin;
      this.idx += 1;
      let par = this.parsePredicateAtom();
      if (par) {
        return new Not(codeOrigin, par);
      }
    }

    if (this.tokens[this.idx].isEqualTo('(')) {
      this.idx += 1;
      this.skipNewLine();
      let result = this.parsePredicate();
      if (this.tokens[this.idx].isNotEqualTo(')')) {
        this.parseError(null);
      }
      this.idx += 1;
      return result;
    }

    if (this.tokens[this.idx].isEqualTo('true')) {
      let result = True.instance();
      this.idx += 1;
      return result;
    }

    if (this.tokens[this.idx].isEqualTo('false')) {
      let result = False.instance();
      this.idx += 1;
      return result;
    }

    if (isIdentifier(this.tokens[this.idx])) {
      let result: Setting = Setting.forName(this.tokens[this.idx].codeOrigin, this.tokens[this.idx].string);
      this.idx += 1;
      return result;
    }
    this.parseError(null);
    return null;
  }

  parsePredicateAnd(): Node | null {
    let result = this.parsePredicateAtom();
    while (this.tokens[this.idx].isEqualTo('and')) {
      let codeOrigin = this.tokens[this.idx].codeOrigin;
      this.idx += 1;
      this.skipNewLine();
      let right = this.parsePredicateAtom();
      if (right != null) {
        result = new And(codeOrigin, result, right);
      }
    }

    return result;
  }

  parsePredicate(): Node | null {
    // some examples of precedence:
    // not a and b -> (not a) and b
    // a and b or c -> (a and b) or c
    // a or b and c -> a or (b and c)

    let result = this.parsePredicateAnd();
    while (this.tokens[this.idx].isEqualTo('or')) {
      let codeOrigin = this.tokens[this.idx].codeOrigin;
      this.idx += 1;
      this.skipNewLine();
      let right = this.parsePredicateAnd();
      if (result != null && right != null) {
        result = new Or(codeOrigin, result, right);
      }
    }

    return result;
  }

  parseVariable(): Node {
    let result = new Node(null);
    if (isIdentifier(this.tokens[this.idx])) {
      result = Variable.forName(this.tokens[this.idx].codeOrigin, this.tokens[this.idx].string);
    } else if (isRegister(this.tokens[this.idx])) {
      if (!fprPattern.test(this.tokens[this.idx].toString())) {
        result = RegisterId.forName(this.tokens[this.idx].codeOrigin, this.tokens[this.idx].string);
      } else {
        result = FpRegisterId.forName(this.tokens[this.idx].codeOrigin, this.tokens[this.idx].string);
      }
    } else {
      this.parseError(null);
    }

    this.idx += 1;
    return result;
  }

  // Resolution address
  parseAddress(offset: Node | null): Node | null {
    if (this.tokens[this.idx].isNotEqualTo('[')) {
      this.parseError(null);
    }

    let codeOrigin = this.tokens[this.idx].codeOrigin;
    let result: Node | null;

    // Three possibilities:
    // []       -> AbsoluteAddress
    // [a]      -> Address
    // [a,b]    -> BaseIndex with scale = 1
    // [a,b,c]  -> BaseIndex

    this.idx += 1;
    if (this.tokens[this.idx].isEqualTo(']')) {
      this.idx += 1;
      return new AbsoluteAddress(codeOrigin, offset);
    }

    let a = this.parseVariable();
    if (this.tokens[this.idx].isEqualTo(']')) {
      result = new Address(codeOrigin, a, offset);
    } else {
      if (this.tokens[this.idx].isNotEqualTo(',')) {
        this.parseError(null);
      }
      this.idx += 1;
      let b = this.parseVariable();
      if (this.tokens[this.idx].isEqualTo(']')) {
        result = new BaseIndex(codeOrigin, a, b, 1, offset);
      } else {
        if (this.tokens[this.idx].isNotEqualTo(',')) {
          this.parseError(null);
        }
        this.idx += 1;
        if (!['1', '2', '4', '8'].includes(this.tokens[this.idx].string)) {
          this.parseError(null);
        }
        let c = Number.parseInt(this.tokens[this.idx].string);
        this.idx += 1;
        if (this.tokens[this.idx].isNotEqualTo(']')) {
          this.parseError(null);
        }
        result = new BaseIndex(codeOrigin, a, b, c, offset);
      }
    }
    this.idx += 1;
    return result!;
  }

  parseColonColon(): Map<string, string[] | CodeOrigin> {
    this.skipNewLine();
    let firstToken = this.tokens[this.idx];
    let codeOrigin = this.tokens[this.idx].codeOrigin;
    if (isIdentifier(firstToken) === false) {
      this.parseError(null);
    }

    let names: string[] = [this.tokens[this.idx].string];
    this.idx += 1;
    while (this.tokens[this.idx].isEqualTo('::')) {
      this.idx += 1;
      if (isIdentifier(this.tokens[this.idx]) === false) {
        this.parseError(null);
      }
      names.push(this.tokens[this.idx].string);
      this.idx += 1;
    }
    if (names.length === 0) {
      firstToken.parseError(null);
    }
    return new Map<string, string[] | CodeOrigin>([
      ['codeOrigin', codeOrigin],
      ['names', names]
    ]);
  }

  parseTextInParens(): Map<string, string[] | CodeOrigin> {
    this.skipNewLine();
    let codeOrigin = this.tokens[this.idx].codeOrigin;
    if (this.tokens[this.idx].isNotEqualTo('(')) {
      print('Missing "(" at ' + codeOrigin);
    }
    this.idx += 1;
    // need at least one item
    if (this.tokens[this.idx].isEqualTo(')')) {
      print('No items in list at ' + codeOrigin);
    }
    let numEnclosedParens: number = 0;
    let text = Array<string>();
    while (this.tokens[this.idx].isNotEqualTo(')') || numEnclosedParens > 0) {
      if (this.tokens[this.idx].isEqualTo('(')) {
        numEnclosedParens += 1;
      } else if (this.tokens[this.idx].isEqualTo(')')) {
        numEnclosedParens -= 1;
      }

      text.push(this.tokens[this.idx].string);
      this.idx += 1;
    }
    this.idx += 1;
    return new Map<string, string[] | CodeOrigin>([
      ['codeOrigin', codeOrigin],
      ['text', text]
    ]);
  }

  parseExpressionAtom(): Node | null {
    let result: Node | null = null;
    this.skipNewLine();
    if (this.tokens[this.idx].isEqualTo('-')) {
      return this.parseTokenForDash();
    }
    if (this.tokens[this.idx].isEqualTo('~')) {
      return this.parseTokenForWavy();
    }
    if (this.tokens[this.idx].isEqualTo('(')) {
      return this.parseTokenForLeft(result);
    }
    if (isInteger(this.tokens[this.idx])) {
      return this.parseTokenForInteger(result);
    }
    if (isString(this.tokens[this.idx])) {
      return this.parseTokenForString(result);
    }
    if (isIdentifier(this.tokens[this.idx])) {
      return this.parseTokenForIdentifier();
    }
    if (isRegister(this.tokens[this.idx])) {
      return this.parseVariable();
    }
    if (this.tokens[this.idx].isEqualTo('sizeof')) {
      return this.parseTokenForSizeof();
    }
    if (this.tokens[this.idx].isEqualTo('constexpr')) {
      return this.parseTokenForConstexpr();
    }
    if (isLabel(this.tokens[this.idx])) {
      return this.parseTokenForLabel(result);
    }
    if (isLocalLabel(this.tokens[this.idx])) {
      return this.parseTokenForLocalLabel(result);
    }
    this.parseError(null);
    return null;
  }

  parseTokenForDash(): Node | null {
    this.idx += 1;
    return new NegImmediate(this.tokens[this.idx - 1].codeOrigin, this.parseExpressionAtom());
  }

  parseTokenForWavy(): Node | null {
    this.idx += 1;
    return new BitnotImmediate(this.tokens[this.idx - 1].codeOrigin, this.parseExpressionAtom());
  }

  parseTokenForLeft(result: Node | null): Node | null {
    this.idx += 1;
    result = this.parseExpression();
    if (this.tokens[this.idx].isNotEqualTo(')')) {
      this.parseError(null);
    }
    this.idx += 1;
    return result;
  }

  parseTokenForInteger(result: Node | null): Node | null {
    result = new Immediate(this.tokens[this.idx].codeOrigin, Number(this.tokens[this.idx].string));
    this.idx += 1;
    return result;
  }

  parseTokenForString(result: Node | null): Node | null {
    result = new StringLiteral(this.tokens[this.idx].codeOrigin, this.tokens[this.idx].string);
    this.idx += 1;
    return result;
  }

  parseTokenForIdentifier(): Node | null {
    let dic = this.parseColonColon();
    let names = dic.get('names') as Array<string>;
    if (names.length > 1) {
      return StructOffset.forField(dic.get('codeOrigin') as CodeOrigin, names.slice(0, -1).join('::'), names[names.length - 1]);
    }
    return Variable.forName(dic.get('codeOrigin') as CodeOrigin, names[0]);
  }

  parseTokenForSizeof(): Node | null {
    this.idx += 1;
    let dic = this.parseColonColon();
    let co = dic.get('codeOrigin') as CodeOrigin;
    let names = dic.get('names') as Array<string>;
    return Sizeof.forName(co, names.join('::'));
  }

  parseTokenForConstexpr(): Node | null {
    this.idx += 1;
    this.skipNewLine();
    let codeOrigin: CodeOrigin;
    let text: Array<string>;
    let names: Array<string>;
    let textStr: string = '';

    if (this.tokens[this.idx].isEqualTo('(')) {
      let dic = this.parseTextInParens();
      codeOrigin = dic.get('codeOrigin') as CodeOrigin;
      text = dic.get('text') as Array<string>;
      textStr = text.join('');
    } else {
      let dic = this.parseColonColon();
      codeOrigin = dic.get('codeOrigin') as CodeOrigin;
      names = dic.get('names') as Array<string>;
      textStr = names.join('::');
    }
    return ConstExpr.forName(codeOrigin, textStr);
  }

  parseTokenForLabel(result: Node | null): Node | null {
    result = new LabelReference(this.tokens[this.idx].codeOrigin, Label.forName(this.tokens[this.idx].codeOrigin, this.tokens[this.idx].string));
    this.idx += 1;
    return result;
  }

  parseTokenForLocalLabel(result: Node | null): Node | null {
    result = new LocalLabelReference(this.tokens[this.idx].codeOrigin, LocalLabel.forName(this.tokens[this.idx].codeOrigin, this.tokens[this.idx].string));
    this.idx += 1;
    return result;
  }

  parseExpressionMul(): Node | null {
    this.skipNewLine();
    let result = this.parseExpressionAtom();
    while (this.tokens[this.idx].isEqualTo('*')) {
      if (this.tokens[this.idx].isEqualTo('*')) {
        this.idx += 1;
        result = new MulImmediates(this.tokens[this.idx - 1].codeOrigin, result, this.parseExpressionAtom());
      } else {
        print('Invalid token ' + this.tokens[this.idx] + ' in multiply expression');
      }
    }
    return result!;
  }

  couldBeExpression(): boolean {
    return (
      this.tokens[this.idx].isEqualTo('-') ||
      this.tokens[this.idx].isEqualTo('~') ||
      this.tokens[this.idx].isEqualTo('constexpr') ||
      this.tokens[this.idx].isEqualTo('sizeof') ||
      isInteger(this.tokens[this.idx]) ||
      isString(this.tokens[this.idx]) ||
      isVariable(this.tokens[this.idx]) ||
      this.tokens[this.idx].isEqualTo('(')
    );
  }

  parseExpressionAdd(): Node | null {
    this.skipNewLine();
    let result = this.parseExpressionMul();
    while (this.tokens[this.idx].isEqualTo('+') || this.tokens[this.idx].isEqualTo('-')) {
      if (this.tokens[this.idx].isEqualTo('+') === true) {
        this.idx += 1;
        result = new AddImmediates(this.tokens[this.idx - 1].codeOrigin, result, this.parseExpressionMul());
      } else if (this.tokens[this.idx].isEqualTo('-')) {
        this.idx += 1;
        result = new SubImmediates(this.tokens[this.idx - 1].codeOrigin, result, this.parseExpressionMul());
      } else {
        print('Invalid token ' + this.tokens[this.idx] + ' in addition expression');
      }
    }
    return result;
  }

  parseExpressionAnd(): Node | null {
    this.skipNewLine();
    let result = this.parseExpressionAdd();
    while (this.tokens[this.idx].isEqualTo('&')) {
      this.idx += 1;
      result = new AndImmediates(this.tokens[this.idx - 1].codeOrigin, result, this.parseExpressionAdd());
    }
    return result;
  }

  parseExpression(): Node | null {
    this.skipNewLine();
    let result = this.parseExpressionAnd();
    while (this.tokens[this.idx].isEqualTo('|') || this.tokens[this.idx].isEqualTo('^')) {
      if (this.tokens[this.idx].isEqualTo('|')) {
        this.idx += 1;
        result = new OrImmediates(this.tokens[this.idx - 1].codeOrigin, result, this.parseExpressionAnd());
      } else if (this.tokens[this.idx].isEqualTo('^')) {
        this.idx += 1;
        result = new XorImmediates(this.tokens[this.idx - 1].codeOrigin, result, this.parseExpressionAnd());
      } else {
        print('Invalid token ' + this.tokens[this.idx] + ' in expression');
      }
    }
    return result;
  }

  parseOperand(comment: string): Node | null {
    this.skipNewLine();
    if (this.couldBeExpression()) {
      let expr = this.parseExpression();
      if (this.tokens[this.idx].isEqualTo('[')) {
        return this.parseAddress(expr);
      }
      return expr;
    }
    if (this.tokens[this.idx].isEqualTo('[')) {
      return this.parseAddress(new Immediate(this.tokens[this.idx].codeOrigin, 0));
    }
    if (isLabel(this.tokens[this.idx])) {
      let result = new LabelReference(this.tokens[this.idx].codeOrigin, Label.forName(this.tokens[this.idx].codeOrigin, this.tokens[this.idx].string));
      this.idx += 1;
      return result;
    }

    if (isLocalLabel(this.tokens[this.idx])) {
      let result = new LocalLabelReference(
        this.tokens[this.idx].codeOrigin,
        LocalLabel.forName(this.tokens[this.idx].codeOrigin, this.tokens[this.idx].string)
      );
      this.idx += 1;
      return result;
    }

    this.parseError(comment);
    return new Node(null);
  }

  parseMacroVariables(): Array<Variable> {
    this.skipNewLine();
    this.consume('^\\($');
    let variables = Array<Variable>();
    while (true) {
      this.skipNewLine();
      if (this.tokens[this.idx].isEqualTo(')')) {
        this.idx += 1;
        break;
      } else if (isIdentifier(this.tokens[this.idx])) {
        variables.push(Variable.forName(this.tokens[this.idx].codeOrigin, this.tokens[this.idx].string));
        this.idx += 1;
        this.skipNewLine();
        if (this.tokens[this.idx].isEqualTo(')')) {
          this.idx += 1;
          break;
        } else if (this.tokens[this.idx].isEqualTo(',') === true) {
          this.idx += 1;
        } else {
          this.parseError(null);
        }
      } else {
        this.parseError(null);
      }
    }
    return variables;
  }

  parseSequence(final: string | null, comment: string | null): Sequence {
    let firstCodeOrigin = this.tokens[this.idx].codeOrigin;
    let list = Array<Node>();
    list = this.convertTokensToList(list, final, comment);
    return new Sequence(firstCodeOrigin, list);
  }

  convertTokensToList(list: Array<Node>, final: string | null, comment: string | null): Array<Node> {
    while (true) {
      if (this.judgeIdxAndFinalTest(final)) {
        break;
      } else if (this.tokens[this.idx] instanceof Annotation) {
        list = this.tokenOfAnnotation(list);
      } else if (this.tokens[this.idx].isEqualTo('\n')) {
        this.idx += 1;
      } else if (this.tokens[this.idx].isEqualTo('const')) {
        list = this.tokenOfConst(list);
      } else if (this.tokens[this.idx].isEqualTo('error')) {
        list.push(new Error(this.tokens[this.idx].codeOrigin));
        this.idx += 1;
      } else if (this.tokens[this.idx].isEqualTo('if') === true) {
        list = this.tokenOfIf(list);
      } else if (this.tokens[this.idx].isEqualTo('macro')) {
        list = this.tokenOfMacro(list);
      } else if (this.tokens[this.idx].isEqualTo('global')) {
        list = this.tokenOfGlobal(list);
      } else if (isInstruction(this.tokens[this.idx])) {
        let codeOrigin = this.tokens[this.idx].codeOrigin;
        let name = this.tokens[this.idx].string;
        this.idx += 1;
        if (this.judgeIdxAndFinal(final)) {
          list = this.tokenOfInstructionForIf(list, codeOrigin, name);
          break;
        } else if (this.tokens[this.idx] instanceof Annotation) {
          list = this.tokenOfInstructionForAnnotation(list, codeOrigin, name);
        } else if (this.tokens[this.idx].isEqualTo('\n')) {
          list = this.tokenOfInstructionForWrap(list, codeOrigin, name);
        } else {
          let endSequence = false;
          list = this.tokenOfInstructionForElse(list, final, endSequence, name, codeOrigin);
          if (endSequence) {
            break;
          }
        }
      } else if (isIdentifier(this.tokens[this.idx])) {
        list = this.tokenOfIdentify(list);
      } else if (this.judgeLabel()) {
        list = this.tokenOfLabel(list);
      } else if (this.tokens[this.idx].isEqualTo('include')) {
        list = this.tokenOfInclude(list);
      } else {
        print('Expecting terminal ' + final + ' ' + comment);
      }
    }
    return list;
  }

  judgeIdxAndFinal(final: string | null): boolean {
    return (final === null && this.idx === this.tokens.length) || (final !== null && RegExp(final).test(this.tokens[this.idx].toString()));
  }

  judgeIdxAndFinalTest(final: string | null): boolean {
    return (this.idx === this.tokens.length && final === null) || (final !== null && RegExp(final).test(this.tokens[this.idx].string));
  }

  judgeLabel(): boolean {
    return isLabel(this.tokens[this.idx]) || isLocalLabel(this.tokens[this.idx]);
  }

  tokenOfAnnotation(list: Array<Node>): Array<Node> {
    // This is the only place where we can encounter a global annotation,
    // and hence need to be able to distinguish between them.
    // globalAnnotations are the ones that start from column 0.
    // All others are considered localAnnotations.
    // The only reason to distinguish between them is so that we can format the output nicely as one would expect.

    let codeOrigin = this.tokens[this.idx].codeOrigin;
    list.push(new Instruction(codeOrigin, 'localAnnotation', [], this.tokens[this.idx].string));
    this.annotation = null;
    this.idx += 2;
    return list;
  }

  tokenOfConst(list: Array<Node>): Array<Node> {
    this.idx += 1;
    if (!isVariable(this.tokens[this.idx])) {
      this.parseError(null);
    }
    let variable: Variable = Variable.forName(this.tokens[this.idx].codeOrigin, this.tokens[this.idx].string);
    this.idx += 1;
    if (this.tokens[this.idx].isNotEqualTo('=')) {
      this.parseError(null);
    }
    this.idx += 1;
    let value = this.parseOperand('while inside of const' + variable.name);
    list.push(new ConstDecl(this.tokens[this.idx].codeOrigin, variable, value));
    return list;
  }

  tokenOfIf(list: Array<Node>): Array<Node> {
    let codeOrigin = this.tokens[this.idx].codeOrigin;
    this.idx += 1;
    this.skipNewLine();
    let predicate = this.parsePredicate();
    this.consume('^((then)|(\n))$');
    this.skipNewLine();
    let ifThenElse = new IfThenElse(codeOrigin, predicate, this.parseSequence('^((else)|(end)|(elsif))$', 'while inside of "if ' + predicate!.dump() + '"'));
    list.push(ifThenElse);
    while (this.tokens[this.idx].isEqualTo('elsif')) {
      let codeOrigin = this.tokens[this.idx].codeOrigin;
      this.idx += 1;
      this.skipNewLine();
      predicate = this.parsePredicate();
      this.consume('^((then)|(\n))$');
      this.skipNewLine();
      if (predicate != null) {
        let elseCase = new IfThenElse(codeOrigin, predicate, this.parseSequence('^((else)|(end)|(elsif))$', 'while inside of "if ' + predicate!.dump() + '"'));
        ifThenElse.elseCase = elseCase;
        ifThenElse = elseCase;
      }
    }
    if (this.tokens[this.idx].isEqualTo('else')) {
      this.idx += 1;
      ifThenElse.elseCase = this.parseSequence('^end$', 'while inside of else case for "if ' + predicate!.dump() + '"');
      this.idx += 1;
    } else {
      if (this.tokens[this.idx].isNotEqualTo('end')) {
        this.parseError(null);
      }
      this.idx += 1;
    }
    return list;
  }

  tokenOfMacro(list: Array<Node>): Array<Node> {
    let codeOrigin = this.tokens[this.idx].codeOrigin;
    this.idx += 1;
    this.skipNewLine();
    if (!isIdentifier(this.tokens[this.idx])) {
      this.parseError(null);
    }
    let name = this.tokens[this.idx].string;
    this.idx += 1;
    let variables = this.parseMacroVariables();
    let body = this.parseSequence('^end$', ('while inside of macro ' + name) as string);
    this.idx += 1;
    list.push(new Macro(codeOrigin, name, variables, body));
    return list;
  }

  tokenOfGlobal(list: Array<Node>): Array<Node> {
    let codeOrigin = this.tokens[this.idx].codeOrigin;
    this.idx += 1;
    this.skipNewLine();
    let name = this.tokens[this.idx].string;
    this.idx += 1;
    Label.setAsGlobal(codeOrigin, name);
    return list;
  }

  tokenOfInstructionForIf(list: Array<Node>, codeOrigin: CodeOrigin, name: string): Array<Node> {
    // Zero operand instruction, and it's the last one.
    list.push(new Instruction(codeOrigin, name, [], this.annotation));
    this.annotation = null;
    return list;
  }

  tokenOfInstructionForAnnotation(list: Array<Node>, codeOrigin: CodeOrigin, name: string): Array<Node> {
    list.push(new Instruction(codeOrigin, name, [], this.tokens[this.idx].string));
    this.annotation = null;
    this.idx += 2; // Consume the newline as well.
    return list;
  }

  tokenOfInstructionForWrap(list: Array<Node>, codeOrigin: CodeOrigin, name: string): Array<Node> {
    // Zero operand instruction.
    list.push(new Instruction(codeOrigin, name, [], this.annotation));
    this.annotation = null;
    this.idx += 1;
    return list;
  }

  tokenOfInstructionForElse(list: Array<Node>, final: string | null, endSequence: boolean, name: string, codeOrigin: CodeOrigin): Array<Node> {
    // It's definitely an instruction, and it has at least one operand.
    let operands = Array<Node | null>();
    this.tokenOfInstructionForElseWhile(operands, final, endSequence, name);
    list.push(new Instruction(codeOrigin, name, operands, this.annotation));
    this.annotation = null;
    return list;
  }

  tokenOfInstructionForElseWhile(operands: Array<Node | null>, final: string | null, endSequence: boolean, name: string): void {
    while (true) {
      operands.push(this.parseOperand('while inside of instruction ' + name));
      if ((!final && this.idx === this.tokens.length) || (final && RegExp(final).test(this.tokens[this.idx].string))) {
        // The end of the instruction and of the sequence.
        endSequence = true;
        break;
      } else if (this.tokens[this.idx].isEqualTo(',')) {
        // Has another operand.
        this.idx += 1;
      } else if (this.tokens[this.idx] instanceof Annotation) {
        this.annotation = this.tokens[this.idx].string;
        this.idx += 2; // Consume the newline as well.
        break;
      } else if (this.tokens[this.idx].isEqualTo('\n')) {
        // The end of the instruction.
        this.idx += 1;
        break;
      } else {
        print('Expected a comma, newline, or ' + final + ' after ' + operands[operands.length - 1]!.dump());
      }
    }
  }

  tokenOfIdentify(list: Array<Node>): Array<Node> {
    // Check macro invocation:
    let codeOrigin = this.tokens[this.idx].codeOrigin;
    let name = this.tokens[this.idx].string;
    this.idx += 1;
    if (this.tokens[this.idx].isEqualTo('(')) {
      list = this.tokenOfIdentifyForLeft(list, name, codeOrigin);
    } else {
      print('Expected "(" after ' + name);
    }
    return list;
  }

  tokenOfIdentifyForLeft(list: Array<Node>, name: string, codeOrigin: CodeOrigin): Array<Node> {
    // Macro invocation.
    this.idx += 1;
    let operands = Array<Node | null>();
    this.skipNewLine();
    if (this.tokens[this.idx].isEqualTo(')')) {
      this.idx += 1;
    } else {
      this.tokenOfIdentifyForElseWhile(operands, name);
    }

    if (this.tokens[this.idx] instanceof Annotation) {
      this.annotation = this.tokens[this.idx].string;
      this.idx += 2;
    }
    list.push(new MacroCall(codeOrigin, name, operands, this.annotation));
    this.annotation = null;

    return list;
  }

  tokenOfIdentifyForElseWhile(operands: Array<Node | null>, name: string): void {
    while (true) {
      this.skipNewLine();
      if (this.tokens[this.idx].isEqualTo('macro')) {
        // It's a macro lambda!
        let codeOriginInner = this.tokens[this.idx].codeOrigin;
        this.idx += 1;
        let variables = this.parseMacroVariables();
        let body = this.parseSequence('^end$', 'while inside of anonymous macro passed as argument to ' + name);
        this.idx += 1;
        operands.push(new Macro(codeOriginInner, '', variables, body));
      } else {
        operands.push(this.parseOperand('while inside of macro call to ' + name));
      }

      this.skipNewLine();
      if (this.tokens[this.idx].isEqualTo(')')) {
        this.idx += 1;
        break;
      } else if (this.tokens[this.idx].isEqualTo(',')) {
        this.idx += 1;
      } else {
        print('Unexpected ' + this.tokens[this.idx].string + ' while parsing invocation of macro ' + name);
      }
    }
  }

  tokenOfLabel(list: Array<Node>): Array<Node> {
    let codeOrigin = this.tokens[this.idx].codeOrigin;
    let name = this.tokens[this.idx].string;
    this.idx += 1;
    if (this.tokens[this.idx].isNotEqualTo(':')) {
      this.parseError(null);
    }
    if (isLabel(name)) {
      list.push(Label.forName(codeOrigin, name, true));
    } else {
      list.push(LocalLabel.forName(codeOrigin, name));
    }
    this.idx += 1;
    return list;
  }

  tokenOfInclude(list: Array<Node>): Array<Node> {
    this.idx += 1;
    if (!isIdentifier(this.tokens[this.idx])) {
      this.parseError(null);
    }
    let moduleName = this.tokens[this.idx].string;
    let fileName = new IncludeFile(moduleName, this.tokens[this.idx].codeOrigin.fileName()).fileName;
    this.idx += 1;
    list.push(parse(fileName));
    return list;
  }

  parseIncludes(final: RegExp, comment: string): Array<string> {
    let firstCodeOrigin = this.tokens[this.idx].codeOrigin;
    let fileList = Array<string>();
    fileList.push(this.tokens[this.idx].codeOrigin.fileName());
    while (true) {
      if ((this.idx === this.tokens.length && !final) || (final && final.test(this.tokens[this.idx].toString()))) {
        break;
      } else if (this.tokens[this.idx].isEqualTo('include')) {
        this.idx += 1;
        if (!isIdentifier(this.tokens[this.idx])) {
          this.parseError(null);
        }
        let moduleName = this.tokens[this.idx].string;
        let fileName = new IncludeFile(moduleName, this.tokens[this.idx].codeOrigin.fileName()).fileName;
        this.idx += 1;
        fileList.push(fileName);
      } else {
        this.idx += 1;
      }
    }

    return fileList;
  }
}

function parseData(data: string, fileName: string): Sequence {
  let parser = new Parser(data, new SourceFile(fileName, null, null));
  return parser.parseSequence(null, '');
}

export function parse(fileName: string): Sequence {
  // debugLog("Load fileName: "+ fileName)
  return parseData(File.open(fileName).read(), fileName);
}
