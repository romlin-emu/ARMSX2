#!/usr/bin/env python3
"""Exercise the actual release shell locally; gh is replaced with a recording stub."""
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
import textwrap
import unittest
from types import SimpleNamespace

WORKFLOW = Path(__file__).resolve().parents[2] / '.github/workflows/romlin_release.yml'
# The final step is a shell block; keep this test dependency-free.
SCRIPT = textwrap.dedent(WORKFLOW.read_text().rsplit('        run: |\n', 1)[1])
ASSET = 'armsx2_libretro-macos-arm64.zip'


class CoreRelease(unittest.TestCase):
    def run_release(self, count, published=False):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'dl').mkdir()
            (root / 'bin').mkdir()
            gh = root / 'bin/gh'
            gh.write_text('#!/bin/bash\n'
                          'if [ "$1 $2" = "release view" ]; then exit "${PUBLISHED_STATUS}"; fi\n'
                          'printf "%s\\n" "$@" > published-args\n')
            gh.chmod(0o755)
            for i in range(count):
                (root / f'dl/core {i}.zip').write_bytes(b'core archive fixture')
            # An unrelated file must not count as a ZIP.
            (root / 'dl/metadata.txt').write_text('metadata')
            env = dict(os.environ, PATH=f'{root / "bin"}:' + os.environ['PATH'],
                       GITHUB_SHA='0123456789abcdef', PUBLISHED_STATUS='0' if published else '1')
            result = subprocess.run(['bash', '-euo', 'pipefail', '-c', SCRIPT],
                                    cwd=root, env=env, capture_output=True, text=True)
            args = root / 'published-args'
            if published:
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertFalse(args.exists())
            elif count != 1:
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('Expected exactly one', result.stdout)
                self.assertFalse(args.exists())
            else:
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn(ASSET + '\n' + ASSET + '.sha256', args.read_text())
                checksum = hashlib.sha256(b'core archive fixture').hexdigest()
                self.assertEqual((root / (ASSET + '.sha256')).read_text(), f'{checksum}  {ASSET}\n')
                self.assertIn('--target\n0123456789abcdef', args.read_text())

    def test_publication_gate(self):
        block = WORKFLOW.read_text().split('  publish:\n', 1)[1]
        expression = ' '.join(block.split('    if: >-\n', 1)[1].split('    name:', 1)[0].split())
        expression = expression.replace('&&', 'and').replace('||', 'or')
        for repository in ('romlin-emu/ARMSX2', 'contributor/ARMSX2'):
            for ref in ('refs/heads/romlin', 'refs/heads/master', 'refs/tags/core-test', 'refs/pull/1/merge'):
                for event in ('push', 'workflow_dispatch', 'pull_request', 'pull_request_target'):
                    with self.subTest(repository=repository, ref=ref, event=event):
                        context = SimpleNamespace(repository=repository, ref=ref, event_name=event)
                        actual = eval(expression, {'__builtins__': {}, 'github': context})
                        expected = (repository == 'romlin-emu/ARMSX2' and ref == 'refs/heads/romlin'
                                    and event in ('push', 'workflow_dispatch'))
                        self.assertEqual(actual, expected)

    def test_zip_count(self):
        for count in (0, 1, 2):
            with self.subTest(count=count):
                self.run_release(count)

    def test_existing_release_is_unchanged(self):
        self.run_release(1, published=True)


if __name__ == '__main__':
    unittest.main()
