#!/usr/bin/env python3
"""
Atomic Commit Analyzer & Staging Helper
Part of the Antigravity atomic-git-commits skill.
Analyzes git working tree status and suggests clean atomic commit groupings.
"""

import subprocess
import sys
import os
from collections import defaultdict
from pathlib import Path

DOMAIN_RULES = [
    {"prefix": ".agents/skills/", "type": "feat", "scope": "skills", "desc": "skills and customization tools"},
    {"prefix": ".antigravity/", "type": "chore", "scope": "rules", "desc": "agent workspace guidelines"},
    {"prefix": "src/core/", "type": "feat", "scope": "core", "desc": "solver and core engine algorithms"},
    {"prefix": "src/app/", "type": "feat", "scope": "ui", "desc": "graphical user interface and window logic"},
    {"prefix": "tests/", "type": "test", "scope": "tests", "desc": "unit and regression tests"},
    {"prefix": "docs/", "type": "docs", "scope": "docs", "desc": "documentation updates"},
    {"prefix": "README", "type": "docs", "scope": "docs", "desc": "documentation updates"},
]

IGNORED_EXTENSIONS = {".exe", ".obj", ".o", ".log", ".tmp", ".bak", ".swp"}

def get_git_status():
    try:
        res = subprocess.run(["git", "status", "--porcelain"], capture_output=True, text=True, check=True)
        return res.stdout.splitlines()
    except subprocess.CalledProcessError as e:
        print(f"Error executing git status: {e}")
        sys.exit(1)
    except FileNotFoundError:
        print("Error: git executable not found.")
        sys.exit(1)

def parse_status(lines):
    files = []
    for line in lines:
        if len(line) < 4:
            continue
        st = line[:2]
        path = line[3:].strip()
        # Handle renames e.g. "R  old -> new"
        if " -> " in path:
            path = path.split(" -> ")[1]
        files.append((st, path))
    return files

def categorize_file(filepath):
    normalized = filepath.replace("\\", "/")

    # Check for temporary/binary files that shouldn't be committed
    p = Path(normalized)
    if p.suffix.lower() in IGNORED_EXTENSIONS:
        return "WARNING_IGNORE", "chore", "build", "temporary or binary artifact"

    for rule in DOMAIN_RULES:
        if normalized.startswith(rule["prefix"]):
            return rule["scope"], rule["type"], rule["scope"], rule["desc"]

    # Fallback by extension
    if p.suffix.lower() in {".md", ".txt", ".rst"}:
        return "docs", "docs", "docs", "documentation files"
    if p.suffix.lower() in {".json", ".xml", ".yml", ".yaml", ".gitignore"}:
        return "config", "chore", "config", "configuration files"

    return "misc", "chore", "general", "miscellaneous modifications"

def main():
    print("=" * 80)
    print("   ATOMIC GIT COMMIT ANALYZER & PLANNER")
    print("=" * 80)

    status_lines = get_git_status()
    if not status_lines:
        print("Working tree clean. No uncommitted changes.")
        return

    parsed = parse_status(status_lines)
    groups = defaultdict(list)
    warnings = []

    for st, path in parsed:
        group_key, c_type, c_scope, desc = categorize_file(path)
        if group_key == "WARNING_IGNORE":
            warnings.append(path)
        else:
            groups[(group_key, c_type, c_scope, desc)].append((st, path))

    if warnings:
        print("\n[!] WARNING: Detected potential build artifacts / temporary files:")
        for w in warnings:
            print(f"    - {w}")
        print("    Consider adding these to .gitignore before committing.\n")

    print(f"Found {len(parsed)} changed files partitioned into {len(groups)} atomic commit milestones:\n")

    milestone_num = 1
    for (group_key, c_type, c_scope, desc), file_list in groups.items():
        print(f"Milestone {milestone_num}: [{c_type}({c_scope})] - {desc}")
        file_args = []
        for st, f in file_list:
            print(f"   [{st}] {f}")
            file_args.append(f'"{f}"')

        add_cmd = f"git add {' '.join(file_args)}"
        commit_cmd = f'git commit -m "{c_type}({c_scope}): <imperative description>"'

        print("\n   Suggested Commands:")
        print(f"   $ {add_cmd}")
        print(f"   $ {commit_cmd}\n")
        print("-" * 80)
        milestone_num += 1

    print("\nChecklist before committing each milestone:")
    print("  [x] Only files related to this single logical change are staged.")
    print("  [x] Code compiles with zero errors.")
    print("  [x] All automated unit tests pass.")
    print("  [x] Commit message uses imperative mood (e.g. 'add', 'fix', 'refactor').")

if __name__ == "__main__":
    main()
