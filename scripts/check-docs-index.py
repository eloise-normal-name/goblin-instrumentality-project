from __future__ import annotations

import re
import sys
from pathlib import Path


def normalize_link_target(target: str) -> str:
    cleaned = target.strip().replace("\\", "/")
    if cleaned.startswith("./"):
        cleaned = cleaned[2:]
    if cleaned.startswith("/"):
        cleaned = cleaned[1:]
    cleaned = cleaned.split("#", 1)[0]
    return cleaned


def collect_markdown_links(text: str) -> set[str]:
    link_pattern = re.compile(r"\[[^\]]*\]\(([^)]+)\)")
    targets = set()
    for match in link_pattern.finditer(text):
        target = match.group(1)
        if target.startswith("http://") or target.startswith("https://"):
            continue
        if target.startswith("mailto:"):
            continue
        normalized = normalize_link_target(target)
        if normalized:
            targets.add(normalized)
    return targets


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    readme_path = repo_root / "README.md"
    docs_dir = repo_root / "docs"

    if not readme_path.exists():
        print("README.md not found. Add a README with a documentation index.")
        return 2

    if not docs_dir.exists():
        print("docs/ directory not found. Nothing to validate.")
        return 0

    readme_text = readme_path.read_text(encoding="utf-8")
    referenced = collect_markdown_links(readme_text)

    docs_files = sorted(docs_dir.rglob("*.md"))
    missing = []
    for doc_path in docs_files:
        relative = doc_path.relative_to(repo_root).as_posix()
        if relative not in referenced:
            missing.append(relative)

    if missing:
        print("The following docs are not referenced from README.md:")
        for path in missing:
            print(f"- {path}")
        print("Add them to the README documentation index.")
        return 1

    print("Docs index check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
