const assert = require('node:assert/strict');
const child_process = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');
const { test } = require('node:test');

const ROOT = path.resolve(__dirname, '..');
// @spec:AC-015
// @spec:AC-016
// @spec:AC-017
// @spec:AC-018
// @spec:AC-019
// @spec:AC-020
// @spec:AC-021
// @spec:AC-022
// @spec:AC-023
// @spec:AC-024
// @spec:AC-025
// @spec:AC-026
const CRITERIA = [
  ['AC-015', 'tests/test_log_format.c'],
  ['AC-016', 'tests/test_log_format.c'],
  ['AC-017', 'tests/test_log_format.c'],
  ['AC-018', 'tests/test_binary_logger.c'],
  ['AC-019', 'tests/test_binary_logger.c'],
  ['AC-020', 'tests/test_binary_logger.c'],
  ['AC-021', 'tests/test_binary_logger.c'],
  ['AC-022', 'tests/test_log_converter.c'],
  ['AC-023', 'tests/test_log_converter.c'],
  ['AC-024', 'tests/test_log_converter.c'],
  ['AC-025', 'tests/test_run_logging.c'],
  ['AC-026', 'tests/test_binary_logger.c'],
];

let harness_result;

function RunHarness() {
  if (!harness_result) {
    const environment = { ...process.env };
    delete environment.NODE_OPTIONS;
    delete environment.NODE_TEST_CONTEXT;
    harness_result = child_process.spawnSync('node', ['test/run_spec_tests.js'], {
      cwd: ROOT,
      encoding: 'utf8',
      env: environment,
    });
  }
  assert.equal(harness_result.status, 0, harness_result.stdout + harness_result.stderr);
}

for (const [criterion, source] of CRITERIA) {
  test(criterion + ': a anotação C e o harness HOST são executáveis @spec:' + criterion, () => {
    assert.match(fs.readFileSync(path.join(ROOT, source), 'utf8'), new RegExp('@spec:' + criterion));
    RunHarness();
  });
}
