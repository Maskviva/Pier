---
layout: home

hero:
  name: Pier
  text: 用你的语言写 LeviLamina 模组
  tagline: 一份 C ABI，一份头文件。模组就是一个会说它的动态库，用什么语言由你定。
  actions:
    - theme: brand
      text: 用 Rust 写模组
      link: /zh/rust/
    - theme: alt
      text: 为什么有 Pier
      link: /zh/guide/why
    - theme: alt
      text: GitHub
      link: https://github.com/Maskviva/pier

features:
  - title: 任何语言
    details: 契约能被 C11 解析。把 FFI 工具指过去，走完四步就行。Rust 是第一个官方绑定，而 Pier 的设计并不偏向它。
    link: /zh/guide/adding-a-language
    linkText: 加一门语言
  - title: 一份布局，通吃所有目标
    details: 函数表里没有条件编译。能力缺席就是槽位为 NULL，所以同一份源码在客户端和服务端宿主上都编得过。
    link: /zh/guide/design
    linkText: 是怎么设计的
  - title: 错误就是错误
    details: 没有任何东西会用一个看起来合理的值，去回答它其实答不上来的问题。「问不出来」和「答案是否」自始至终分开。
    link: /zh/guide/design
    linkText: 背后的理由
  - title: 契约只增不减
    details: 新增能力就是追加一个槽。按老版本 Pier 编的模组照常能用，而会破坏它的变更会在装载时被明确拒绝并说明原因。
    link: /zh/guide/compatibility
    linkText: 兼容性
---
