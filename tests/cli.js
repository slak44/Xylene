'use strict';
const cp = require('child_process');
const fs = require('fs');
const chalk = require('chalk');
const async = require('async');

let successfulTests = 0;

fs.stat('../Lang', function (err, stats) {
  if (err) {
    console.error(chalk.red('Could not stat executable'));
    return;
  }
  runTests();
});

function equalityTest(name, expectedOutput, command) {
  return function (callback) {
    cp.exec(`../Lang ${command}`, function (err, stdout, stderr) {
      if (err) {
        testFinished(name, `${err.message}\nSignal: ${chalk.red(err.signal)}`, callback);
        return;
      }
      if (stdout === expectedOutput) testFinished(name, callback);
      else testFinished(name, `Command: ${command}\nExpected output: ${expectedOutput}\nRecieved output: ${stdout}`, callback);
    });
  };
}

let tests = {
  integers: equalityTest('Integers', '1', '-e "print(1)"'),
  strings: equalityTest('Strings', 'qwerty1234{}/*-~?', '-e \'print("qwerty1234{}/*-~?")\''),
  booleans: equalityTest('Booleans', 'true', '-e \'print(true)\''),
  floats: equalityTest('Floats', '1.2', '-e \'print(1.2)\''),
  intPlus: equalityTest('Integer Addition', '16', '-e \'print(8 + 8)\''),
  fltPlus: equalityTest('Float Addition', '16.32', '-e \'print(8.16 + 8.16)\''),
  strPlus: equalityTest('String Concatenation', 'abcdef', '-e \'print("abc" + "def")\''),
  assignment: equalityTest('Basic Assignment', '123abc', '-e \'define a = 123; print(a); a = "abc"; print(a);\''),
  multiTypeDef: equalityTest('Multiple Types', '03.14',
  `-e '
  Integer, Float number = 0;
  print(number);
  number = 3.14;
  print(number);
  '`),
  booleanCond: equalityTest('Boolean Expression', 'false',
  `-e '
  !(true || false && true || false)
  '`),
  ifTest: equalityTest('Simple Conditional', '42',
  `-e '
  if true do
    print(42);
  end
  '`),
  ifComplexTest: equalityTest('Full Conditional', '42',
  `-e '
  if true do
    print(42);
  else
    print(420);
  end
  '`),
};

function testFinished(testName, errorString, callback) {
  if (arguments.length == 2) callback = errorString;
  if (errorString && typeof errorString === 'string') {
    console.error(`Test '${chalk.yellow(testName)}' ${chalk.red('failed')}\n${errorString}`);
    callback();
    return;
  }
  console.log(`Test '${chalk.yellow(testName)}' ok ${chalk.green('✓')}`);
  successfulTests++;
  callback();
}

function runTests() {
  let testCount = Object.keys(tests).length;
  async.parallel(tests, function (err) {
    if (successfulTests === testCount) {
      console.log(chalk.underline.green('All tests ok!'));
    } else {
      console.log(`${chalk.green(successfulTests)} tests ok.\n${chalk.red(String(testCount - successfulTests))} tests failed`);
    }
  });
}
