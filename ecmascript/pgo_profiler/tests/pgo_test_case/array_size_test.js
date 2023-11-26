function foo() {
  let a = [1, 2, 3, 4];
}

function foo1() {
  let a = [1, 2, 3, 4];
  a[7] = 10;
}

function foo2() {
  return [1, 2, 3, 4];
}
foo();
foo1();

let a = foo2();
a[7] = 10;