import { defineConfig } from 'vitepress'

// The site is general to specific. `/guide/` is Pier itself: where it came from, how it
// is designed, and how a language gets bound to it. `/rust/` is the first official
// binding, and is where someone who just wants to write a mod should land.
//
// The Chinese locale mirrors that structure under `/zh/`. Code comments stay English
// everywhere per contract §7; a page marked as Chinese is a translation, not a comment.

const enNav = [
  { text: 'Pier', link: '/guide/what-is-pier' },
  { text: 'Rust', link: '/rust/' },
  {
    text: 'Bindings',
    items: [
      { text: 'Rust (official)', link: '/rust/' },
      { text: 'Add a language', link: '/guide/adding-a-language' },
    ],
  },
  { text: 'Releases', link: 'https://github.com/Maskviva/pier/releases' },
]

const enSidebar = {
  '/guide/': [
    {
      text: 'Pier',
      items: [
        { text: 'What is Pier', link: '/guide/what-is-pier' },
        { text: 'Why it exists', link: '/guide/why' },
        { text: 'How it is designed', link: '/guide/design' },
        { text: 'Installing', link: '/guide/installation' },
      ],
    },
    {
      text: 'Binding a language',
      items: [
        { text: 'Adding a language', link: '/guide/adding-a-language' },
        { text: 'The ABI', link: '/guide/abi' },
        { text: 'The manifest', link: '/guide/manifest' },
        { text: 'Compatibility', link: '/guide/compatibility' },
      ],
    },
  ],
  '/rust/': [
    {
      text: 'Getting started',
      items: [
        { text: 'Overview', link: '/rust/' },
        { text: 'Your first mod', link: '/rust/first-mod' },
        { text: 'The mod lifecycle', link: '/rust/lifecycle' },
      ],
    },
    {
      text: 'Writing mods',
      items: [
        { text: 'Events', link: '/rust/events' },
        { text: 'Commands', link: '/rust/commands' },
        { text: 'Errors and logging', link: '/rust/errors' },
        { text: 'Threads', link: '/rust/threads' },
        { text: 'Talking to other mods', link: '/rust/cross-mod' },
      ],
    },
    { text: 'Reference', items: [{ text: 'API map', link: '/rust/api' }] },
  ],
}

const zhNav = [
  { text: 'Pier', link: '/zh/guide/what-is-pier' },
  { text: 'Rust', link: '/zh/rust/' },
  {
    text: '语言绑定',
    items: [
      { text: 'Rust（官方）', link: '/zh/rust/' },
      { text: '加一门语言', link: '/zh/guide/adding-a-language' },
    ],
  },
  { text: '发布', link: 'https://github.com/Maskviva/pier/releases' },
]

const zhSidebar = {
  '/zh/guide/': [
    {
      text: 'Pier',
      items: [
        { text: 'Pier 是什么', link: '/zh/guide/what-is-pier' },
        { text: '为什么有 Pier', link: '/zh/guide/why' },
        { text: '是怎么设计的', link: '/zh/guide/design' },
        { text: '安装', link: '/zh/guide/installation' },
      ],
    },
    {
      text: '绑定一门语言',
      items: [
        { text: '加一门语言', link: '/zh/guide/adding-a-language' },
        { text: 'ABI', link: '/zh/guide/abi' },
        { text: 'manifest', link: '/zh/guide/manifest' },
        { text: '兼容性', link: '/zh/guide/compatibility' },
      ],
    },
  ],
  '/zh/rust/': [
    {
      text: '起步',
      items: [
        { text: '总览', link: '/zh/rust/' },
        { text: '第一个模组', link: '/zh/rust/first-mod' },
        { text: '模组生命周期', link: '/zh/rust/lifecycle' },
      ],
    },
    {
      text: '写模组',
      items: [
        { text: '事件', link: '/zh/rust/events' },
        { text: '命令', link: '/zh/rust/commands' },
        { text: '错误与日志', link: '/zh/rust/errors' },
        { text: '线程', link: '/zh/rust/threads' },
        { text: '跨模组通信', link: '/zh/rust/cross-mod' },
      ],
    },
    { text: '参考', items: [{ text: 'API 地图', link: '/zh/rust/api' }] },
  ],
}

const shared = {
  socialLinks: [{ icon: 'github', link: 'https://github.com/Maskviva/pier' }],
  search: { provider: 'local' as const },
}

export default defineConfig({
  title: 'Pier',
  base: '/',
  cleanUrls: true,
  lastUpdated: true,

  // A contributor runbook, not a page of the site.
  srcExclude: ['verify-delayload.md'],

  head: [['meta', { name: 'theme-color', content: '#62B47A' }]],

  locales: {
    root: {
      label: 'English',
      lang: 'en-US',
      description: 'A LeviLamina mod loader for languages other than C++.',
      themeConfig: {
        ...shared,
        nav: enNav,
        sidebar: enSidebar,
        editLink: {
          pattern: 'https://github.com/Maskviva/pier/edit/main/docs/:path',
          text: 'Edit this page on GitHub',
        },
        footer: {
          message: 'Released under the Apache-2.0 License.',
          copyright:
            'Not affiliated with Mojang, Microsoft or LeviMC. Minecraft is a trademark of Mojang Synergies AB.',
        },
      },
    },

    zh: {
      label: '简体中文',
      lang: 'zh-CN',
      description: '让 C++ 之外的语言也能写 LeviLamina 模组的装载器。',
      themeConfig: {
        ...shared,
        nav: zhNav,
        sidebar: zhSidebar,
        editLink: {
          pattern: 'https://github.com/Maskviva/pier/edit/main/docs/:path',
          text: '在 GitHub 上编辑此页',
        },
        docFooter: { prev: '上一页', next: '下一页' },
        outline: { label: '本页目录' },
        lastUpdatedText: '最后更新',
        returnToTopLabel: '回到顶部',
        darkModeSwitchLabel: '外观',
        sidebarMenuLabel: '菜单',
        footer: {
          message: '以 Apache-2.0 许可发布。',
          copyright:
            '与 Mojang、Microsoft、LeviMC 无关。Minecraft 是 Mojang Synergies AB 的商标。',
        },
      },
    },
  },
})
