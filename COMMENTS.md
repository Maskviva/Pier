# The comment standard

The C++ comment standard of Pier. This document expands `CONTRACT.md` §7, and where the two
conflict this one governs.

---

## 0. The criterion

**A comment only writes what the code cannot answer.** Everything else is deleted,
including the parts that really were useful when they were written.

Once released, the reader of a comment is a stranger and not someone who took part in that
discussion. Any comment that needs knowing what happened at the time is a negative for a
stranger: they have to read through a sequence of events that does not concern them before
reaching the one sentence that does.

Three consequences, and the rest of this document expands them:

1. **Write the constraint, not the sequence of events.** "A client always requests from
   subchunk -32" is a constraint; "we measured it three times, the first sent -64..320..."
   is a sequence of events. The first stays and the second goes.
2. **Write the present, not the past.** The history of the code is in git and one
   `git log -L` is more accurate than any comment. History inside a comment only rots,
   because it is not edited along with the code.
3. **One comment, one reason.** Not being able to finish means the code should be split,
   not that the comment should grow.

---

## 1. Three levels and their hard budgets

| Level | Position | Budget | What it answers |
|---|---|---|---|
| L1 file header | the block comment at the top of a file | at most 16 lines | what this TU is responsible for; the constraints a reader has to know first |
| L2 declaration comment | the `/** */` above a declaration | at most 14 lines | what a caller has to observe |
| L3 body comment | consecutive `//` lines above a statement | at most 8 lines | why this place is not written the obvious way |

The budget includes the `/**` and `*/` lines. Over budget does not mean written in detail,
it means put at the wrong level: design goes in `CONTRACT.md`, history goes in git, and a
pending item goes in an issue.

One file is exempt: `packages/pier-abi/include/sdk/abi.h`, see §6.

The bindings side, `bindings/rust`, is bound only by the budgets of §1, measured by
`rust_comment_budget.py`. The rustdoc conventions inside `///` and `//!`, meaning
`# Safety`, `# Panics` and fenced example code, are structure that renders into
documentation and are not treated as the markdown-in-a-comment of §3.5.

The first line of an L1 has a fixed shape and fits on one line:

```cpp
/** <file name>: <one sentence of responsibility>. */
```

When the responsibility is one sentence, that is the end of it. A file header is not
expanded merely so that one exists.

---

## 2. Only these five kinds stay

1. **Constraints**: threads, locks, re-entry, lifetime, ownership, call order. A reader who
   does not observe them gets it wrong, and the code itself does not state them.
2. **Counter-intuitive facts**: where the actual behavior of the engine or the platform
   disagrees with its documentation or with intuition; where a magic number comes from.
3. **Danger**: a place that fails silently when deleted or optimized in passing. It has to
   state the symptom, because a symptom can be searched for and a conclusion cannot.
4. **Contract**: ABI stability rules, ownership transfer across the boundary, what is
   returned on failure.
5. **A rejected obvious approach**, and only where it is obvious enough that the next
   person will certainly try it. One line saying why it does not work, with no account of
   the attempt.

A comment outside these five kinds is one nobody will need once it is deleted.

---

## 3. Seven kinds that are always deleted

1. **Restating the code.** A "take the lock" next to a `std::lock_guard`.
2. **Narrating development.** "the first version", "originally", "used to", "the old
   repository", "rewritten three times", "this passage was paid for in blood".
3. **Internal ticket numbers.** A review round number of any form. They point at a document
   the reader cannot get. A `stage N` is not one of these, being the teardown order of
   `spi::TeardownReg` and a real concept in the code.
4. **Conversational person and rhetoric.** "we", "you", "the next person will...", "the
   tempting half-measure". A comment is a statement and not persuasion.
5. **Markdown layout inside a comment.** A leading `#` heading, a fenced code block, a
   `|---|` table, bold. A comment is not a documentation site, and needing a heading to
   break it up says it is over budget.
6. **A change log.** "changed to", "migrated", "added". That belongs to git.
7. **Pending items and open questions.** `TODO`, `FIXME`, "perhaps this should". Those
   belong in an issue.

---

## 4. Register

- Declarative sentences, third person, present tense. Write the properties of the code and
  not the actions of its author.
- One thing per sentence. No subordinate clause inserted with a dash, and no
  not-X-but-Y construction.
- No emphatic formatting. A sentence that needs bold to stand up is rewritten.
- Terms use the original names of the engine and the ABI, such as `Player::attack` and
  `PierApi::struct_size`, with no alias invented.
- **Language: every comment is in English, with no exception anywhere in the repository**,
  covering the implementation side, the contract header, the build scripts and the tool
  scripts alike. The reason shares its root with the reason §6 gives for `abi.h`: the
  readers of a comment are binding authors in any language and server operators in any
  region, and a comment in one natural language turns a constraint anyone can read into a
  constraint only readers of that language can read.
- Spelling is American, as in `behavior`, `initialize` and `deserializer`, and is not mixed
  within one repository.

A comparison:

```cpp
// Delete: We first hooked PlayerUseItemEvent, then guessed at PlayerThrowProjectileEvent
//         (which does not exist on the LL bus), and only later found that vanilla
//         projectiles are item components now...
// Keep:   Vanilla projectiles are item components and throwing goes through
//         ThrowableItemComponent::_doThrow. Spawner::spawnProjectile is not on the
//         player path.
```

---

## 5. Format

- A declaration uses `/** ... */` and a body uses `//`. A body uses no block comment.
- Every line of a multi-line block comment starts with ` * `, indented to align with the
  declaration it comments.
- Line width is at most 100 columns, including the indentation and the ` * `.
- A trailing comment is separated from the code by one space, as in `int x = 3; // reason`.
  A trailing comment does not wrap.
- A blank line is not used to separate paragraphs; needing paragraphs says it is over
  budget.
- No ASCII rules are drawn.

---

## 6. The exception for `pier-abi/include/sdk/abi.h`

That header is the product itself and not an implementation file: it is at the same time
the reference documentation of the SDK in every language, and its readers have nothing
else. Therefore:

- **It is not bound by the L1 and L2 budgets.** The meaning of each slot, what its
  parameters mean, what it returns on failure and what it requires of threads are written
  in full, because one missing sentence leaves a binding author in some language guessing.
- **It is still bound by all of §3, §4 and §5.** In particular: no account of development,
  no ticket numbers, no markdown layout. The language follows §4, the same as every other
  file.
- No spelling of any consumer language appears, which the `abi-no-lang` check guards. A C++
  spelling is permitted only while explaining a host-side mechanism.

The same exception does not apply to any header outside `pier-abi`.

---

## 7. A comment must not lie about the code

This inherits `CONTRACT.md` §5.4 and has the highest priority while trimming:

- A comment claiming a safety property, such as the exception being caught, nothing being
  thrown, or server thread only, has to sit next to the code implementing that property.
  Where that is impossible the comment changes, not its wording.
- **Trimming must not turn a true statement into a false one.** Deleting one qualifier,
  such as after the first call or while the lock is held, often turns an accurate sentence
  into a wrong one. Before deleting, confirm that what remains stands on its own.
- An empty `catch` block keeps one line of comment saying why it may be empty. It is the
  only checkable evidence between a deliberate fallback and an omission, and the
  `no-silent-fallback` check depends on it.

---

## 8. Machine checks

```bash
python3 tools/checks/comment_style.py
```

It covers the budgets of §1, items 1, 2, 3, 5, 6 and 7 of §3, and the line width and ASCII
rules of §5.

**The language item of §4 currently has a machine check only on `abi.h`.** The CJK test in
the script is bound to the `is_abi` branch and the banned-word list is entirely regular
expressions over one natural language, so after the repository was translated that list
matches nothing. Both need to follow, and until they do the language item has only a
`grep` for CJK characters as a backstop, which under contract §9 means a delivery note may
not give it a checkmark.

**What it cannot cover**: whether a comment is true (§7), whether it is one of the five
kinds of §2, and whether it restates the code. Those three need a human. A passing script
means the mechanical rules hold and not that this standard holds, and under
`CONTRACT.md` §9 a delivery note may only copy that sentence.
