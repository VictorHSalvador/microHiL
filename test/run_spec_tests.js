const assert = require('node:assert/strict');
const child_process = require('node:child_process');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');

const ROOT = path.resolve(__dirname, '..');
const tests = [];

function RunCommand(command, arguments_list) {
  return child_process.spawnSync(command, arguments_list, {
    cwd: ROOT,
    encoding: 'utf8',
  });
}

function CommandOutput(result) {
  return (result.stdout || '') + (result.stderr || '');
}

function ReadTapResults(output) {
  const results = [];
  for (const line of output.split(/\r?\n/)) {
    const match = line.match(/^\s*(not )?ok\s+\d+\s*(?:-\s*)?(.*)$/);
    if (match) results.push({ title: match[2].trim(), pass: !match[1] });
  }
  return results;
}

function CaptureExistingRunner() {
  const output = [];
  const original_log = console.log;

  console.log = (...arguments_list) => output.push(arguments_list.join(' '));
  try {
    require(path.join(ROOT, 'test', 'sdd-versioning.spec.test.js'));
  } finally {
    console.log = original_log;
  }

  return output.join('\n');
}

function RegisterTest(title, body) {
  tests.push({ title, body });
}

function WithTemporaryBuild(body) {
  const build_directory = fs.mkdtempSync(path.join(os.tmpdir(), 'microhil-ac-009-'));
  try {
    return body(build_directory);
  } finally {
    fs.rmSync(build_directory, { recursive: true, force: true });
  }
}

const existing_output = CaptureExistingRunner();
for (const result of ReadTapResults(existing_output)) {
  tests.push(result);
}

if (process.exitCode) {
  tests.push({
    title: 'O runner de especificação existente terminou com falha',
    pass: false,
  });
}

RegisterTest('AC-009: Os testes HOST são descobertos e executados @spec:AC-009', () => {
  WithTemporaryBuild((build_directory) => {
    const configure = RunCommand('cmake', [
      '-S', ROOT,
      '-B', build_directory,
      '-DMICROHIL_BUILD_RUNNER=OFF',
      '-DBUILD_TESTING=ON',
    ]);
    assert.equal(configure.status, 0, CommandOutput(configure));

    const build = RunCommand('cmake', ['--build', build_directory]);
    assert.equal(build.status, 0, CommandOutput(build));

    const discovery = RunCommand('ctest', ['--test-dir', build_directory, '-N']);
    assert.equal(discovery.status, 0, CommandOutput(discovery));
    const discovered_tests = CommandOutput(discovery);
    assert.match(discovered_tests, /sample_queue_spsc_order/);
    assert.match(discovered_tests, /binary_logger_sink_failures/);
    assert.match(discovered_tests, /log_converter_csv/);
    assert.match(discovered_tests, /run_logging_enabled/);
    assert.match(discovered_tests, /run_logging_disabled/);
    assert.match(discovered_tests, /run_logging_saturation/);
    assert.match(discovered_tests, /run_logging_failure/);
    assert.match(discovered_tests, /run_logging_descriptor/);
    assert.match(discovered_tests, /run_logging_aggregate/);

    const execution = RunCommand('ctest', ['--test-dir', build_directory, '--output-on-failure']);
    assert.equal(execution.status, 0, CommandOutput(execution));
    assert.match(CommandOutput(execution), /100% tests passed, 0 tests failed out of [1-9][0-9]*/);
  });
});

console.log('TAP version 13');
let failures = 0;
for (const [index, current_test] of tests.entries()) {
  try {
    if (current_test.body) current_test.body();
    if (current_test.pass === false) throw new Error('test runner reported failure');
    console.log('ok ' + (index + 1) + ' - ' + current_test.title);
  } catch (error) {
    failures++;
    console.log('not ok ' + (index + 1) + ' - ' + current_test.title);
    console.log('  ---');
    console.log('  message: ' + JSON.stringify(error.message));
    console.log('  ...');
  }
}
console.log('1..' + tests.length);
if (failures > 0) process.exitCode = 1;
