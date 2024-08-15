/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

// 1. sendable can only inherit from sendable
try {
  class SendableParent {
    constructor() {
      'use sendable';
    }
  }
  class SendableChild extends SendableParent {
    constructor() {
      'use sendable';
      super();
    }
  }
  print('sendable inherit from sendable succeed.');
} catch (err) {
  print('sendable inherit from sendable failed. err: ' + err + ', code: ' + err.code);
}
try {
  class Parent {
    constructor() {}
  }
  class SendableChild extends Parent {
    constructor() {
      'use sendable';
      super();
    }
  }
  print('sendable inherit from sendable succeed.');
} catch (err) {
  print('sendable inherit from non-sendable failed. err: ' + err + ', code: ' + err.code);
}

// 2. non-sendable can not inherit from sendable
try {
  class Parent {
    constructor() {}
  }
  class Child extends Parent {
    constructor() {
      super();
    }
  }
  print('non-sendable inherit from non-sendable succeed.');
} catch (err) {
  print('non-sendable inherit from non-sendable failed. err: ' + err + ', code: ' + err.code);
}
try {
  class SendableParent {
    constructor() {
      'use sendable';
    }
  }
  class Child extends SendableParent {
    constructor() {
      super();
    }
  }
  print('non-sendable inherit from sendable succeed.');
} catch (err) {
  print('non-sendable inherit from sendable failed. err: ' + err + ', code: ' + err.code);
}

// 3. non-sendable can not implement sendable

// 4. sendable can only contain sendable
try {
  class A {
    constructor() {
      'use sendable';
    }
  }
  class B {
    a: A | null = null;
    constructor(a: A) {
      'use sendable';
      this.a = a;
    }
  }
  let b = new B(new A());
  print('sendable contain sendable succeed.');
} catch (err) {
  print('sendable contain sendable failed. err: ' + err + ', code: ' + err.code);
}
try {
  class A {
    constructor() {}
  }
  class B {
    a: A | null = null;
    constructor(a: A) {
      'use sendable';
      this.a = a;
    }
  }
  let b = new B(new A());
  print('sendable contain non-sendable succeed.');
} catch (err) {
  print('sendable contain non-sendable failed. err: ' + err + ', code: ' + err.code);
}

// 5. template type of collections must be sendable

// 6. sendable can not use ! assertion

// 7. sendable can not use computeed property names

// 8. sendable can not use variables in current context
try {
  class B {
    constructor() {
      'use sendable';
    }
  }
  function bar(): B {
    return new B();
  }
  class C {
    constructor() {
      'use sendable';
    }
    v: B = new B();
    u: B = bar();
    foo() {
      return new B();
    }
  }
  print('sendable contain lexenv succeed.');
} catch (err) {
  print('sendable contain lexenv failed. err: ' + err + ', code: ' + err.code);
}

// 9. sendable can not use decorator except @sendable
try {
  class A {
    constructor() {
      'use sendable';
      'use concurrent';
    }
  }
  class B {
    constructor() {
      'use sendable';
    }
    foo() {
      'use concurrent';
    }
  }
  print('sendable with concurrent decorator succeed.');
} catch (err) {
  print('sendable with concurrent decorator failed. err: ' + err + ', code: ' + err.code);
}

// 10. sendable can not be initial with object or array

// 11. non-sendable can not be `as` sendable