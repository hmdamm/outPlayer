# tests/test_project_scaffold.py
import os
import sys

required_files = [
    "TASKS.md",
    "docs/decisions.md",
    "docs/clean-room-log.md",
    "THIRD_PARTY_NOTICES.md",
    ".gitignore",
    "README.md",
]

for f in required_files:
    if not os.path.exists(f):
        print(f"Missing required file: {f}")
        sys.exit(1)
print("Scaffold verification passed.")
