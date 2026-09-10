const assert = require('node:assert/strict');
const child_process = require('node:child_process');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const tests = [];

const ROOT = path.resolve(__dirname, '..');
const VERSION_FILE = path.join(ROOT, 'docs', 'sdd-versions.md');
const FEATURE_FILE = path.join(ROOT, '.spec', 'features', 'sdd-versioning', 'spec.md');

function Read(relative_path) {
  return fs.readFileSync(path.join(ROOT, relative_path), 'utf8');
}

function ListMarkdown(directory) {
  const files = [];
  for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
    if (entry.name === '.git' || entry.name === 'node_modules') continue;
    const entry_path = path.join(directory, entry.name);
    if (entry.isDirectory()) files.push(...ListMarkdown(entry_path));
    else if (entry.isFile() && entry.name.endsWith('.md')) files.push(entry_path);
  }
  return files;
}

function HistoryRows(versioning) {
  return versioning.split(/\r?\n/).filter((line) => /^\| 0\.\d+\.\d+ \|/.test(line)).map((line) => line.split('|').slice(1, -1).map((cell) => cell.trim()));
}

function BrokenLocalLinks(files) {
  const broken = [];
  for (const file of files) {
    const content = fs.readFileSync(file, 'utf8');
    for (const match of content.matchAll(/\[[^\]]+\]\(([^)]+)\)/g)) {
      let target = match[1].trim().replace(/^<|>$/g, '').split('#', 1)[0];
      if (!target || /^[a-z]+:\/\//i.test(target) || target.startsWith('codex:')) continue;
      target = decodeURIComponent(target);
      const resolved = path.isAbsolute(target) ? target : path.resolve(path.dirname(file), target);
      if (!fs.existsSync(resolved)) broken.push(path.relative(ROOT, file) + ' -> ' + target);
    }
  }
  return broken;
}

function RegisterTest(title, body) {
  tests.push({ title, body });
}

function WithTemporaryBuild(label, body) {
  const build_directory = fs.mkdtempSync(path.join(os.tmpdir(), label + '-'));
  try {
    return body(build_directory);
  } finally {
    fs.rmSync(build_directory, { recursive: true, force: true });
  }
}

function RunCmake(arguments_list, environment = process.env) {
  return child_process.spawnSync('cmake', arguments_list, {
    cwd: ROOT,
    encoding: 'utf8',
    env: environment,
  });
}

RegisterTest('AC-001: A versão vigente é identificável @spec:AC-001', () => {
  const index = Read('docs/README.md');
  const versioning = Read('docs/sdd-versions.md');
  const version_files = fs.readdirSync(path.join(ROOT, 'docs')).filter((name) => /^sdd-versions\.md$/.test(name));
  assert.deepEqual(version_files, ['sdd-versions.md']);
  assert.match(index, /\[sdd-versions\.md\]\(sdd-versions\.md\)/);
  assert.match(versioning, /Versão vigente: \*\*SDD-MICROHIL 0\.15\.1\*\*, de 10\.09\.2026/);
  assert.match(versioning, /Status da versão:/);
});

RegisterTest('AC-002: O estado separa especificação, implementação e verificação @spec:AC-002', () => {
  const versioning = Read('docs/sdd-versions.md');
  assert.match(versioning, /\| Especificação \|/);
  assert.match(versioning, /\| Implementação \|/);
  assert.match(versioning, /\| Verificação \|/);
  assert.match(versioning, /não executa FMU, ROS, USB, firmware, GUI, Raspberry Pi, bancada, HIL ou timing real/i);
});

RegisterTest('AC-003: Cada revisão possui metadados mínimos @spec:AC-003', () => {
  const rows = HistoryRows(Read('docs/sdd-versions.md'));
  assert.equal(rows.length, 23);
  for (const row of rows) {
    assert.equal(row.length, 7);
    assert.ok(row.every((cell) => cell.length > 0));
  }
  assert.deepEqual(rows.map((row) => row[0]), ['0.1.0', '0.2.0', '0.3.0', '0.4.0', '0.5.0', '0.6.0', '0.7.0', '0.7.1', '0.7.2', '0.7.3', '0.7.4', '0.8.0', '0.8.1', '0.9.0', '0.10.0', '0.11.0', '0.11.1', '0.11.2', '0.13.0', '0.13.1', '0.14.0', '0.15.0', '0.15.1']);
});

RegisterTest('AC-004: O procedimento de atualização é explícito @spec:AC-004', () => {
  const versioning = Read('docs/sdd-versions.md');
  assert.match(versioning, /## Esquema de versão/);
  assert.match(versioning, /\*\*MAJOR:\*\*/);
  assert.match(versioning, /\*\*MINOR:\*\*/);
  assert.match(versioning, /\*\*PATCH:\*\*/);
  assert.match(versioning, /## Procedimento de atualização/);
  assert.match(versioning, /requisito em spec, decisão em ADR\/decisions, interface no ICD/);
  assert.match(versioning, /registrar commit quando criado/i);
});

RegisterTest('AC-005: Requisitos e matriz permanecem alinhados @spec:AC-005', () => {
  const specification = Read('docs/spec.md');
  const traceability = Read('docs/traceability.md');
  const requirement_ids = [...specification.matchAll(/^### (REQ-(?:F|NF)-\d{2})\b/gm)].map((match) => match[1]);
  const trace_ids = [...traceability.matchAll(/^\| (REQ-(?:F|NF)-\d{2}) \|/gm)].map((match) => match[1]);
  assert.equal(requirement_ids.length, 59);
  assert.equal(new Set(requirement_ids).size, 59);
  assert.deepEqual([...requirement_ids].sort(), [...trace_ids].sort());
  const markdown_files = [path.join(ROOT, 'README.md'), path.join(ROOT, 'AGENTS.md'), ...ListMarkdown(path.join(ROOT, 'docs')), ...ListMarkdown(path.join(ROOT, '.spec'))];
  assert.deepEqual(BrokenLocalLinks(markdown_files), []);
});

RegisterTest('AC-006: A camada mecânica não duplica requisitos do produto @spec:AC-006', () => {
  const feature = fs.readFileSync(FEATURE_FILE, 'utf8');
  const constitution = Read('.spec/constituicao.md');
  assert.match(feature, /fonte normativa dos requisitos do produto continua sendo docs\/spec\.md/i);
  assert.match(constitution, /requisitos REQ-F e REQ-NF permanecem em docs\/spec\.md/);
  assert.equal((feature.match(/^### REQ-(?:F|NF)-/gm) || []).length, 0);
});

RegisterTest('P-002: Existe uma única especificação normativa do produto @principle:P-002', () => {
  const feature = fs.readFileSync(FEATURE_FILE, 'utf8');
  assert.equal((feature.match(/^### REQ-(?:F|NF)-/gm) || []).length, 0);
  assert.match(Read('docs/sdd-versions.md'), /docs\/spec\.md\]\(spec\.md\) é a única especificação normativa/);
});

RegisterTest('P-003: Estado documental não é estado de implementação @principle:P-003', () => {
  const versioning = Read('docs/sdd-versions.md');
  assert.match(versioning, /TASK-001 e TASK-002 HOST implementadas, evidenciadas e auditadas mecanicamente; validação do produto pendente/);
  assert.match(versioning, /Aprovação documental não significa implementação/);
});

RegisterTest('P-004: Toda revisão do SDD é rastreável @principle:P-004', () => {
  const rows = HistoryRows(Read('docs/sdd-versions.md'));
  assert.ok(rows.length > 0);
  assert.ok(rows.every((row) => row.length === 7 && row.every(Boolean)));
});

RegisterTest('AC-007: O build independente configura em diretório limpo @spec:AC-007', () => {
  WithTemporaryBuild('microhil-ac-007', (build_directory) => {
    const result = RunCmake([
      '-S', ROOT,
      '-B', build_directory,
      '-DMICROHIL_BUILD_RUNNER=OFF',
      '-DBUILD_TESTING=ON',
    ]);
    assert.equal(result.status, 0, result.stdout + result.stderr);
    assert.ok(fs.existsSync(path.join(build_directory, 'CMakeCache.txt')));

    const targets = RunCmake(['--build', build_directory, '--target', 'help']);
    assert.equal(targets.status, 0, targets.stdout + targets.stderr);
    assert.match(targets.stdout, /microhil_host_components/);
  });
});

RegisterTest('AC-008: Os componentes independentes compilam com os avisos do projeto @spec:AC-008', () => {
  WithTemporaryBuild('microhil-ac-008', (build_directory) => {
    const configure = RunCmake([
      '-S', ROOT,
      '-B', build_directory,
      '-DMICROHIL_BUILD_RUNNER=OFF',
      '-DBUILD_TESTING=ON',
      '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
    ]);
    assert.equal(configure.status, 0, configure.stdout + configure.stderr);

    const build = RunCmake(['--build', build_directory, '--target', 'microhil_host_components']);
    assert.equal(build.status, 0, build.stdout + build.stderr);

    const commands = JSON.parse(fs.readFileSync(path.join(build_directory, 'compile_commands.json'), 'utf8'));
    const own_sources = commands.filter((entry) => entry.file.startsWith(path.join(ROOT, 'src') + path.sep));
    assert.ok(own_sources.length >= 4);
    for (const command of own_sources) {
      assert.match(command.command, /-Wall/);
      assert.match(command.command, /-Wextra/);
      assert.match(command.command, /-Wpedantic/);
      assert.match(command.command, /-Wshadow/);
      assert.match(command.command, /-Wconversion/);
    }
  });
});

RegisterTest('AC-010: A ausência da dependência é informada somente quando necessária @spec:AC-010', () => {
  WithTemporaryBuild('microhil-ac-010', (build_directory) => {
    const environment = { ...process.env };
    delete environment.FMILIB_ROOT;
    const result = RunCmake([
      '-S', ROOT,
      '-B', build_directory,
      '-DMICROHIL_BUILD_RUNNER=ON',
      '-DBUILD_TESTING=OFF',
      '-DFMILIB_INCLUDE_DIR=FMILIB_INCLUDE_DIR-NOTFOUND',
      '-DFMILIB_LIBRARY=FMILIB_LIBRARY-NOTFOUND',
      '-DCMAKE_FIND_ROOT_PATH=' + path.join(build_directory, 'no-fmilib'),
      '-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY',
      '-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY',
    ], environment);
    assert.notEqual(result.status, 0);
    const output = result.stdout + result.stderr;
    assert.match(output, /MICROHIL_BUILD_RUNNER=ON/);
    assert.match(output, /-DFMILIB_ROOT=<prefix>/);
    assert.match(output, /-DMICROHIL_FETCH_FMILIB=ON/);
  });
});

RegisterTest('AC-011: A versão oficial selecionada é imutável para o build @spec:AC-011', () => {
  const cmake = Read('cmake/fmilib.cmake');
  assert.match(cmake, /set\(MICROHIL_FMILIB_VERSION "3\.0\.4"\)/);
  assert.match(cmake, /set\(MICROHIL_FMILIB_REVISION "[0-9a-f]{40}"\)/);
  assert.match(cmake, /GIT_REPOSITORY https:\/\/github\.com\/modelon-community\/fmi-library\.git/);
  assert.match(cmake, /GIT_TAG \$\{MICROHIL_FMILIB_REVISION\}/);
  assert.match(cmake, /FetchContent_MakeAvailable\(microhil_fmilib\)/);
  assert.match(cmake, /set\(\$\{result_target\} fmilib_shared PARENT_SCOPE\)/);
});

RegisterTest('AC-012: Uma instalação explícita continua suportada @spec:AC-012', () => {
  const cmake = Read('cmake/fmilib.cmake');
  assert.match(cmake, /set\(FMILIB_ROOT "" CACHE PATH/);
  assert.match(cmake, /set\(FMILIB_INCLUDE_DIR "" CACHE PATH/);
  assert.match(cmake, /set\(FMILIB_LIBRARY "" CACHE FILEPATH/);
  assert.match(cmake, /if\(FMILIB_INCLUDE_DIR AND FMILIB_LIBRARY\)/);
  assert.match(cmake, /IMPORTED_LOCATION "\$\{FMILIB_LIBRARY\}"/);
  assert.match(cmake, /INTERFACE_INCLUDE_DIRECTORIES "\$\{FMILIB_INCLUDE_DIR\}"/);
  assert.match(cmake, /integrator must prove its version and compatibility/);
});

RegisterTest('AC-013: A documentação permite repetir os builds @spec:AC-013', () => {
  const readme = Read('README.md');
  const evidence = Read('docs/evidence/host-build-foundation-2026-09-07.md');
  assert.match(readme, /-DMICROHIL_BUILD_RUNNER=OFF -DBUILD_TESTING=ON/);
  assert.match(readme, /-DMICROHIL_BUILD_RUNNER=ON -DMICROHIL_FETCH_FMILIB=ON/);
  assert.match(readme, /ctest --test-dir \/tmp\/microhil-host-independent --output-on-failure/);
  assert.match(evidence, /Ubuntu 22\.04, GCC 11\.4\.0/);
  assert.match(evidence, /FMILibrary 3\.0\.4, revisão imutável `4a4b21ec10a632b2768a604c2330c54204919644`/);
  assert.match(evidence, /17\/17 testes com sucesso/);
  assert.match(evidence, /Nenhuma FMU foi executada/);
});

RegisterTest('AC-014: O SDD reflete o estado observado @spec:AC-014', () => {
  const plan = Read('docs/plan.md');
  const task = Read('docs/tasks/TASK-001.md');
  const traceability = Read('docs/traceability.md');
  const gates = Read('docs/quality-gates.md');
  const versioning = Read('docs/sdd-versions.md');
  const task_two = Read('docs/tasks/TASK-002.md');
  const logging_feature = Read('.spec/features/host-run-logging/spec.md');
  const logging_tasks = Read('.spec/features/host-run-logging/tasks.md');
  assert.match(plan, /TASK-002[\s\S]*Concluída, evidenciada e auditada/);
  assert.match(task_two, /Status: concluída, evidenciada e auditada mecanicamente/);
  assert.match(task_two, /Nenhuma FMU foi executada/);
  assert.match(logging_feature, /> status: pronta/);
  for (const task_id of ['T-005', 'T-006', 'T-007', 'T-008', 'T-009']) assert.match(logging_tasks, new RegExp(task_id + '[\\s\\S]*\\[concluida\\]'));
  assert.match(task, /Status: concluída, evidenciada e auditada/);
  assert.match(task, /Executar uma FMU/);
  for (const requirement_id of ['REQ-NF-01', 'REQ-NF-11', 'REQ-NF-13', 'REQ-NF-18']) {
    const row = traceability.split(/\r?\n/).find((line) => line.startsWith('| ' + requirement_id + ' |'));
    assert.ok(row && row.includes('host-build-foundation-2026-09-07.md'));
  }
  assert.match(gates, /- \[x\] Resultado agregado e integridade de dados coerentes/);
  assert.match(versioning, /\| 0\.7\.3 \| 09\.09\.2026 \| PATCH \|/);
  for (const requirement_id of ['REQ-F-07', 'REQ-F-09', 'REQ-F-11', 'REQ-F-12', 'REQ-F-22', 'REQ-F-25', 'REQ-F-26', 'REQ-NF-30']) {
    const row = traceability.split(/\r?\n/).find((line) => line.startsWith('| ' + requirement_id + ' |'));
    assert.ok(row && row.includes('host-run-logging-2026-09-08.md'));
  }
});

console.log('TAP version 13');
let failures = 0;
for (const [index, current_test] of tests.entries()) {
  try {
    current_test.body();
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
