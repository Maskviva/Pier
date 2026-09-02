# Why Pier exists

Pier started as [levilamina-rust-loader](https://github.com/Maskviva/levilamina-rust-loader),
built to answer one question: can a LeviLamina mod be written in Rust?

It could. And answering it surfaced a harder problem, which is what Pier is for.

## What the loader got wrong

The loader grew its interface by accretion, one function at a time as each was needed.
That works until the shape underneath matters, and then four things went wrong that could
not be patched out.

### The layout forked per build target

Conditional blocks inside the function table meant its tail sat at a different offset on a
client build than on a server one.

A marker in the high bits of the version number was added to compensate. It could not
protect a mod nobody had rebuilt, so a mod compiled before the fork kept loading and every
slot past the divergence landed on the wrong function pointer. Both sides compiled
cleanly; the symptom was a call doing something unrelated.

One misalignment reached seven slots. Nothing reported it, because nothing could.

### The center called out to every capability

The dispatch table named each package's functions directly. Every one of those names is a
link edge from the center out to a capability, so removing a capability from a build broke
the link.

"Optional" was therefore a word in the documentation and not a property of the code. The
packages documented as optional could not actually be dropped.

### Missing values became plausible ones

A field that could not be read came back as a zero, and a zero is a legitimate answer for
most of the things being read.

The clearest case: a land protection mod could not read the dimension out of an event
payload, wrote `unwrap_or(0)`, and every such event was treated as happening in the
overworld. Protection refused inside the overworld and allowed everything everywhere else.
Nothing was logged. The behaviour looked deliberate for as long as nobody tested a custom
dimension.

### Event names matched on substrings

An event id was resolved with a substring search. The day upstream added an event sharing
a stem with a synthetic one, every subscriber to the real event would be silently
redirected to a synthetic event with a different payload shape, with no route back.

Nobody hit that one. It was waiting.

## What that adds up to

Each of those is fixable in isolation, and several were patched. Together they say
something a patch cannot fix: the interface was never designed, only grown, and the
patches were defending a shape that should not have existed.

The loader was a test. It did what a test is for, which is to tell you what to build
properly. It is not maintained.

## What Pier does differently

Pier inverts the order. The header is written first and the implementation second, and
each of the four problems above becomes a structural property rather than a rule to
remember:

| The problem | What replaces it |
|---|---|
| Layout forked per target | One layout on every target; an absent capability is a NULL slot |
| The center called out to capabilities | Capabilities register inward through an SPI; dropping one changes no host code |
| Missing became plausible | Nothing answers with a value it could not determine; cannot-determine and no stay apart |
| Substring matching | An exact id or a namespaced suffix, one matcher, no fallthrough |

And one rule above those: a property nothing checks is a wish. Each of them has a script
in `tools/checks/` that runs on every push, and each script states what it covers and what
it cannot see.

[How it is designed](./design) goes through the mechanisms.
