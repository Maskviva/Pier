---
layout: home

hero:
  name: Pier
  text: LeviLamina mods, in your language
  tagline: One C ABI, one header. A mod is a dynamic library that speaks it, and the language is your choice.
  actions:
    - theme: brand
      text: Write a mod in Rust
      link: /rust/
    - theme: alt
      text: Why Pier exists
      link: /guide/why
    - theme: alt
      text: GitHub
      link: https://github.com/Maskviva/pier

features:
  - title: Any language
    details: The contract parses as C11. Point an FFI tool at one header and follow three steps. Rust is the first official binding; nothing about Pier prefers it.
    link: /guide/adding-a-language
    linkText: Add a language
  - title: One layout, every target
    details: No conditional compilation in the function table. An absent capability is a NULL slot, so the same source builds for a client and a server host alike.
    link: /guide/design
    linkText: How it is designed
  - title: Errors stay errors
    details: Nothing answers a question it could not determine with a plausible value. Cannot-be-determined and no are kept apart all the way down.
    link: /guide/design
    linkText: The reasoning
  - title: The contract only grows
    details: Adding a capability appends a slot. A mod built against an older Pier keeps working, and a change that would break it is refused at load with a message saying so.
    link: /guide/compatibility
    linkText: Compatibility
---
