#!/usr/bin/env python3
"""Show full representative SKILL.md text for validating a file_sha from the review sheet.
Usage: python tools/show_skill.py agent_skills_sample.db <40-hex-file-sha>
"""
import re
import sqlite3
import sys

if len(sys.argv) != 3 or not re.fullmatch(r'[a-fA-F0-9]{40}', sys.argv[2]):
    raise SystemExit(__doc__)
with sqlite3.connect(f'file:{sys.argv[1]}?mode=ro', uri=True) as db:
    row = db.execute('''SELECT repo_full_name, path, content FROM artifacts
                        WHERE file_sha = ? AND dedup_primary = 1 AND content IS NOT NULL
                        LIMIT 1''', (sys.argv[2],)).fetchone()
    if not row:
        raise SystemExit('No representative content found for that file_sha')
    print(f'Repository: {row[0]}\nPath: {row[1]}\n' + '-'*72 + '\n' + row[2])
