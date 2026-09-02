import { defineConfig } from 'vitepress'

// The site is general to specific. `/guide/` is Pier itself: where it came from, how it
// is designed, and how a language gets bound to it. `/rust/` is the first official
// binding, and is where someone who just wants to write a mod should land.
export default defineConfig({
  title: 'Pier',
  description: 'A LeviLamina mod loader for languages other than C++.',
  lang: 'en-US',
  base: '/pier/',
  cleanUrls: true,
  lastUpdated: true,

  // A contributor runbook, not a page of the site.
  srcExclude: ['verify-delayload.md'],

  head: [['meta', { name: 'theme-color', content: '#62B47A' }]],

  themeConfig: {
    nav: [
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
    ],

    sidebar: {
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
        {
          text: 'Reference',
          items: [{ text: 'API map', link: '/rust/api' }],
        },
      ],
    },

    socialLinks: [{ icon: 'github', link: 'https://github.com/Maskviva/pier' }],

    editLink: {
      pattern: 'https://github.com/Maskviva/pier/edit/main/docs/:path',
      text: 'Edit this page on GitHub',
    },

    search: { provider: 'local' },

    footer: {
      message: 'Released under the Apache-2.0 License.',
      copyright:
        'Not affiliated with Mojang, Microsoft or LeviMC. Minecraft is a trademark of Mojang Synergies AB.',
    },
  },
})
