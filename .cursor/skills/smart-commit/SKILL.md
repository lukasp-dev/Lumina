---
name: smart-commit
description: >-
  Prepares sensible git commits: scopes changes, writes clear conventional
  messages, splits or squashes when appropriate, and verifies before commit.
  Use when staging, committing, amending, writing commit messages, or when the
  user asks for a smart or clean commit.
---

# Smart commit

## Goals

- One commit = one coherent story (reviewers and `git bisect` can follow history).
- Messages explain **why** when the **what** is not obvious from the diff title.
- No accidental noise: secrets, debug prints, formatter-only churn mixed with logic unless intentional.

## Before writing the message

1. Inspect the actual change set: `git status`, `git diff` (and `git diff --staged` if partial staging).
2. If unrelated changes are mixed, recommend **split commits** (stage by path or with `git add -p`) instead of one vague message.
3. If the diff is huge, suggest a **single focused commit** with a summary line plus body bullets for major areas—only split when chunks are truly independent.

## Message format

Prefer [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<optional-scope>): <short imperative summary>

[optional body: motivation, tradeoffs, breaking changes]
```

- **Types**: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`, `perf`, `ci`, `build`, etc.
- **Summary**: imperative mood (~50 chars), no trailing period, no vague words ("update", "fix stuff").
- **Body**: wrap ~72 chars; mention breaking changes or migration notes explicitly.

## When the user only says "commit"

1. Propose a message **after** reading the diff, not from guesswork.
2. Offer the exact `git` commands (or run them if the user wants execution): stage → verify diff → commit.
3. Call out **risky** changes (auth, migrations, public API) and suggest a slightly longer body.

## Anti-patterns to avoid

- Commit messages that only repeat the filename.
- Mixing refactors with behavior changes without separation or a clear body.
- Force-pushing shared branches unless the user explicitly asked and understands the impact.
