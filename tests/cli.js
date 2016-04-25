'use strict';
const cp = require('child_process');
const fs = require('fs');
const chalk = require('chalk');
const async = require('async');

let successfulTests = 0;
let testMessages = [];

fs.stat('../Lang', function (err, stats) {
  if (err) {
    console.error(chalk.red('Could not stat executable'));
    return;
  }
  runTests();
});

function equalityTest(name, expectedOutput, command) {
  return function (callback) {
    let child = cp.exec(`../Lang ${command}`, function (err, stdout, stderr) {
      if (err) {
        testFinished(name, `${err.message}\nSignal: ${chalk.red(err.signal)}`, callback);
        return;
      }
      if (stdout === expectedOutput) testFinished(name, callback);
      else testFinished(name, `Command: ${command}\nExpected output: ${expectedOutput}\nRecieved output: ${stdout}`, callback);
    });
    setTimeout(() => {
      child.kill('SIGKILL');
    }, 5 * 1000 /* Kill it after 5 seconds */);
  };
}

let tests = {
  turingComplete: equalityTest('Turing Completeness', '11111\n10001\n10011\n', `-e '
  // Rule 110
  Integer timesToRun = 3;
  String input = "01101";
  input = "0" + input + "0";
  Integer initInputSize = input.length();
  while timesToRun-- > 0 do
    String newState = "";
    Integer i = 0;
    while i <= initInputSize - 3 do
      String result = input.substr(i, 3);
      i++;
      if result == "000" || result == "100" || result == "111" do
        newState += "0";
      else
        newState += "1";
      end
    end
    input = "0" + newState + "0";
    print(newState + "\n");
  end
  '`),
  escapechars: equalityTest('Escape Characters', '\n,\\,\b,J,T',
  `-e '
  print("\\n,\\\\,\\b,\\x4A,\\124")
  '`),
  integers: equalityTest('Literals, Integer', '1 255 5 89',
  `-e '
  print(1);
  print(" ");
  print(0xFF);
  print(" ");
  print(0b101);
  print(" ");
  print(0o131);
  '`),
  strings: equalityTest('Literals, String', 'qwerty1234{}/*-~?',
  `-e '
  print("qwerty1234{}/*-~?")
  '`),
  booleans: equalityTest('Literals, Boolean', 'true',
  `-e '
  print(true)
  '`),
  floats: equalityTest('Literals, Float', '1.2',
  `-e '
  print(1.2)
  '`),
  intPlus: equalityTest('Addition, Integer', '16',
  `-e '
  print(8 + 8)
  '`),
  fltPlus: equalityTest('Addition, Float', '16.32',
  `-e '
  print(8.16 + 8.16)
  '`),
  strPlus: equalityTest('Strings, Concatenation', 'abcdef',
  `-e '
  print("abc" + "def")
  '`),
  strLen: equalityTest('Strings, Length', '9',
  `-e '
  print("123456789".length())
  '`),
  strSub: equalityTest('Strings, Substr', 'sd',
  `-e '
  String aaa = "asdfghjkl";
  print(aaa.substr(1, 2));
  '`),
  assignment: equalityTest('Assignment, Basic', '123abc',
  `-e '
  define a = 123;
  print(a); a = "abc";
  print(a);
  '`),
  multiTypeDef: equalityTest('Types, Multiple', '03.14',
  `-e '
  Integer, Float number = 0;
  print(number);
  number = 3.14;
  print(number);
  '`),
  booleanCond: equalityTest('Expressions, Boolean', 'false',
  `-e '
  print(!(true || false && true || false));
  '`),
  ifTest: equalityTest('Conditionals, Simple', '42',
  `-e '
  if true do
    print(42);
  end
  '`),
  ifComplexTest: equalityTest('Conditionals, Full', '42',
  `-e '
  if true do
    print(42);
  else
    print(420);
  end
  '`),
  whileLoop: equalityTest('Loops, While', '0123',
  `-e '
  Integer i = 0;
  while i < 4 do
    print(i++);
  end
  '`),
  increment: equalityTest('Operators, Increment', '10',
  `-e '
  Integer i = 0;
  print(++i);
  i = 0;
  print(i++);
  '`),
  decrement: equalityTest('Operators, Decrement', '-10',
  `-e '
  Integer i = 0;
  print(--i);
  i = 0;
  print(i--);
  '`),
  basicFunc: equalityTest('Functions, Basic', '1',
  `-e '
  define function printOne do
    print(1);
  end
  printOne();
  '`),
  argumentFunc: equalityTest('Functions, Arguments', '4.2',
  `-e '
  define function add [Integer, Float a] [Integer, Float b] do
    print(a + b);
  end
  add(1, 3.2);
  '`),
  returnFunc: equalityTest('Functions, Return Values', '4',
  `-e '
  define function random => Integer do
    return 4; // Chosen by fair dice roll
  end
  print(random());
  '`),
  fullFunc: equalityTest('Functions, Complete', '4.2',
  `-e '
  define function add [Integer, Float a] [Integer, Float b] => Integer, Float do
    return a + b;
  end
  print(add(1, 3.2));
  '`),
  prefixArgs: equalityTest('Functions, Prefix Arguments', '4.2',
  `-e '
  define function add [a: Integer, Float] [b: Integer, Float] do
    print(a + b);
  end
  add(1, 3.2);
  '`),
  comments1: equalityTest('Comments, Line', '123',
  `-e '
  // print(1234);
  print(123);
  '`),
  comments2: equalityTest('Comments, Multi-Line', '123',
  `-e '
  /* print(1234);
  print(90);
  */
  print(123);
  '`),
  expressions1: equalityTest('Expressions, Simple', '124',
  `-e '
  print((123 + 123) / 2 + 2 >> 1);
  '`),
  expressions2: equalityTest('Expressions, Function', '0',
  `-e '
  print("abc".length() - 3);
  '`),
};

function testFinished(testName, errorString, callback) {
  if (arguments.length == 2) callback = errorString;
  if (errorString && typeof errorString === 'string') {
    testMessages.push(`Test '${chalk.yellow(testName)}' ${chalk.red('failed')}\n${errorString}`);
    callback();
    return;
  }
  testMessages.push(`Test '${chalk.yellow(testName)}' ok ${chalk.green('✓')}`);
  successfulTests++;
  callback();
}

function runTests() {
  let testCount = Object.keys(tests).length;
  async.parallel(tests, function (err) {
    console.log(testMessages.sort().join('\n'));
    if (successfulTests === testCount) {
      console.log(chalk.underline.green('All tests ok!'));
    } else {
      console.log(`${chalk.green(successfulTests)} tests ok\n${chalk.red(String(testCount - successfulTests))} tests failed`);
    }
    // If we got here, it means no test is still alive, so the parent can kill itself as well
    process.exit(0);
  });
}
