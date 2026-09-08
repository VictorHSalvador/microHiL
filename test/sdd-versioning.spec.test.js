const assert = require('node:assert/strict');
const fs = require('node:fs');
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

RegisterTest('AC-001: A versão vigente é identificável @spec:AC-001', () => {
  const index = Read('docs/README.md');
  const versioning = Read('docs/sdd-versions.md');
  const version_files = fs.readdirSync(path.join(ROOT, 'docs')).filter((name) => /^sdd-versions\.md$/.test(name));
  assert.deepEqual(version_files, ['sdd-versions.md']);
  assert.match(index, /\[sdd-versions\.md\]\(sdd-versions\.md\)/);
  assert.match(versioning, /Versão vigente: \*\*SDD-MICROHIL 0\.7\.1\*\*, de 07\.09\.2026/);
  assert.match(versioning, /Status da versão:/);
});

RegisterTest('AC-002: O estado separa especificação, implementação e verificação @spec:AC-002', () => {
  const versioning = Read('docs/sdd-versions.md');
  assert.match(versioning, /\| Especificação \|/);
  assert.match(versioning, /\| Implementação \|/);
  assert.match(versioning, /\| Verificação \|/);
  assert.match(versioning, /não executa FMU, ROS, USB, firmware, GUI, bancada ou HIL/i);
});

RegisterTest('AC-003: Cada revisão possui metadados mínimos @spec:AC-003', () => {
  const rows = HistoryRows(Read('docs/sdd-versions.md'));
  assert.equal(rows.length, 8);
  for (const row of rows) {
    assert.equal(row.length, 7);
    assert.ok(row.every((cell) => cell.length > 0));
  }
  assert.deepEqual(rows.map((row) => row[0]), ['0.1.0', '0.2.0', '0.3.0', '0.4.0', '0.5.0', '0.6.0', '0.7.0', '0.7.1']);
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
  assert.match(versioning, /baseline documental consolidada e auditável; implementação do produto pendente/);
  assert.match(versioning, /Aprovação documental não significa implementação/);
});

RegisterTest('P-004: Toda revisão do SDD é rastreável @principle:P-004', () => {
  const rows = HistoryRows(Read('docs/sdd-versions.md'));
  assert.ok(rows.length > 0);
  assert.ok(rows.every((row) => row.length === 7 && row.every(Boolean)));
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
