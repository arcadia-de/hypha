# Changelog

## [1.11.0](https://github.com/arcadia-de/hypha/compare/v1.10.0...v1.11.0) (2026-09-04)


### Features

* ✨ add diff infrastructure ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* ✨ finish all status & diff functions for Controllers ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* ✨ implement PackageManager controller status ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* ✨ partially implement the controllers and some tests ([47a8806](https://github.com/arcadia-de/hypha/commit/47a8806f56752bb4b060567903f970a9fc241d77))
* **ctrl/archive:** ✨ implement Archive controller diff fn ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* **ctrl/archive:** ✨ implement ArchiveController and some tests ([47a8806](https://github.com/arcadia-de/hypha/commit/47a8806f56752bb4b060567903f970a9fc241d77))
* **ctrl/directory:** ✨ implement Directory Controller and some code cleanup ([47a8806](https://github.com/arcadia-de/hypha/commit/47a8806f56752bb4b060567903f970a9fc241d77))
* **ctrl/directory:** ✨ implement Directory controller diff fn ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* **ctrl/download:** ✨ add a Download Controller, some tests and code cleanup ([47a8806](https://github.com/arcadia-de/hypha/commit/47a8806f56752bb4b060567903f970a9fc241d77))
* **ctrl/download:** ✨ implement Download controller diff fn ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* **ctrl/package-manager:** ✨ implement PackageManager Controller and some code cleanup ([47a8806](https://github.com/arcadia-de/hypha/commit/47a8806f56752bb4b060567903f970a9fc241d77))
* **ctrl/package-manager:** ✨ implement PackageManager diff fn ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* **ctrl/package:** ✨ implement Package Controller and some code cleanup ([47a8806](https://github.com/arcadia-de/hypha/commit/47a8806f56752bb4b060567903f970a9fc241d77))
* **ctrl/package:** ✨ implement Package controller diff fn ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* **ctrl/package:** ✨ implement Package controller status fn ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* **ctrl/repository:** ✨ implement Repository controller and tests ([47a8806](https://github.com/arcadia-de/hypha/commit/47a8806f56752bb4b060567903f970a9fc241d77))
* **ctrl/repository:** ✨ implement Repository controller diff fn ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* **ctrl/symlink:** ✨ impl Symlink Controller, some tests and code cleanup ([47a8806](https://github.com/arcadia-de/hypha/commit/47a8806f56752bb4b060567903f970a9fc241d77))
* **ctrl/symlink:** ✨ implement Symlink controller diff fn ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* **ctrl/task:** ✨ implement Task controller diff fn ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* **ctrl/task:** ✨ implement Task controller status fn ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* **ctrl/template:** ✨ implement Template controller diff fn ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* **ctrl/template:** ✨ implement Template Controller, some tests and code cleanup ([47a8806](https://github.com/arcadia-de/hypha/commit/47a8806f56752bb4b060567903f970a9fc241d77))
* **ctrl/test:** ✨ implement Task controller status fn ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* **ctrl/test:** ✨ implement Test controller diff fn ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* **ctrl/test:** ✨ implement Test controller status fn ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* **jsonnet-lib:** ✨ improve jsonnet bindings [#56](https://github.com/arcadia-de/hypha/issues/56) ([ac92e54](https://github.com/arcadia-de/hypha/commit/ac92e543975ab9f928e90e0358ca67b8fc4bf63b))
* **jsonnet-lib:** ✨ improve jsonnet lib natives ([ac92e54](https://github.com/arcadia-de/hypha/commit/ac92e543975ab9f928e90e0358ca67b8fc4bf63b))


### Bug Fixes

* 🐛 fix resource.h extern ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* **ctrl/download:** 🐛 fix download Controller compilation error ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))
* **utils/process:** 🐛 fix Process reaping happening after waitpid so status can be set properly ([a4f3170](https://github.com/arcadia-de/hypha/commit/a4f3170f5723b501d954e6a1337ce5fda03c224c))

## [1.10.0](https://github.com/arcadia-de/hypha/compare/v1.9.0...v1.10.0) (2026-09-01)


### Features

* ✨ add tldr docs generator ([a1c77b4](https://github.com/arcadia-de/hypha/commit/a1c77b4e8c1617c9ea8314bce478b590c9af69f5))
* **gql:** ✨ add manifests collection to query schemas ([341eb52](https://github.com/arcadia-de/hypha/commit/341eb5269e50ac25b8c203d6dcd7a564704efbd1))
* **query:** ✨ make query command report in different formats ([341eb52](https://github.com/arcadia-de/hypha/commit/341eb5269e50ac25b8c203d6dcd7a564704efbd1))


### Bug Fixes

* **resource-graph:** 🐛 fix ResourceBootstrapTest failing due to faulty logic in FindProvides algo ([15fa9da](https://github.com/arcadia-de/hypha/commit/15fa9dadf7c6be0aa527f94e1435bc71bd328f38))

## [1.9.0](https://github.com/arcadia-de/hypha/compare/v1.8.0...v1.9.0) (2026-08-31)


### Features

* ✨ add code for .hyphaignore files ([671c101](https://github.com/arcadia-de/hypha/commit/671c1016ca7d6ad16bf7822fd33d51ccee30ab1d))
* ✨ add code for .hyphaignore files ([b14afcd](https://github.com/arcadia-de/hypha/commit/b14afcd38bfd32eb6ab53cf5fffcea2f8f7dd183))
* ✨ add Manifests as a read-only Resource kind ([38bf2ed](https://github.com/arcadia-de/hypha/commit/38bf2edc4f9049e359cd7ed7978faa653215b1d3))
* ✨ add manifests as a Resource kind ([38bf2ed](https://github.com/arcadia-de/hypha/commit/38bf2edc4f9049e359cd7ed7978faa653215b1d3)), closes [#48](https://github.com/arcadia-de/hypha/issues/48)
* **hyphaignore:** ✨ add ignore class for fnmatching against files from a .hyphaignore ([b14afcd](https://github.com/arcadia-de/hypha/commit/b14afcd38bfd32eb6ab53cf5fffcea2f8f7dd183)), closes [#46](https://github.com/arcadia-de/hypha/issues/46)


### Bug Fixes

* 🐛 fix warning in expander test ([38bf2ed](https://github.com/arcadia-de/hypha/commit/38bf2edc4f9049e359cd7ed7978faa653215b1d3))

## [1.8.0](https://github.com/arcadia-de/hypha/compare/v1.7.0...v1.8.0) (2026-08-30)


### Features

* ✨ add a Task resource and controller ([e1bb9fb](https://github.com/arcadia-de/hypha/commit/e1bb9fb7fddc5893247903918cb5717dbc57566d))
* ✨ add Task resources for ad-hoc commands ([e1bb9fb](https://github.com/arcadia-de/hypha/commit/e1bb9fb7fddc5893247903918cb5717dbc57566d))


### Bug Fixes

* 🐛 fix tasks ([e1bb9fb](https://github.com/arcadia-de/hypha/commit/e1bb9fb7fddc5893247903918cb5717dbc57566d))

## [1.7.0](https://github.com/arcadia-de/hypha/compare/v1.6.0...v1.7.0) (2026-08-30)


### Features

* ✨ cleanup docker builds and add a sandbox image ([336936a](https://github.com/arcadia-de/hypha/commit/336936a2500c063e6c0ba6b7ee6e186ec06b44fd))

## [1.6.0](https://github.com/arcadia-de/hypha/compare/v1.5.0...v1.6.0) (2026-08-30)


### Features

* ✨ force another version bump for release please ([85a0319](https://github.com/arcadia-de/hypha/commit/85a03199b53a3290185fc0518f43b2faa964f1f9))

## [1.5.0](https://github.com/arcadia-de/hypha/compare/1.4.0...v1.5.0) (2026-08-30)


### Features

* **README:** ✨ add missing git.yaml manifest from example and force a version bump from previous commits ([454d468](https://github.com/arcadia-de/hypha/commit/454d468b0bc3abf7b2dc401a016fe4daf83a71b8))

## [1.4.0](https://github.com/arcadia-de/hypha/compare/1.3.0...v1.4.0) (2026-08-29)


### Features

* ✨ make discovery happen automatically when no sources specified ([1f7cdf0](https://github.com/arcadia-de/hypha/commit/1f7cdf0a91dedde7dc03f49ec9870be154b415fb))

## [1.3.0](https://github.com/arcadia-de/hypha/compare/v1.2.0...v1.3.0) (2026-08-29)


### Features

* ✨ get apply log visible in the console ([60eae5d](https://github.com/arcadia-de/hypha/commit/60eae5db6e38b7d11829c787e37c0e297e9ae32b))


### Bug Fixes

* 🐛 fix RunModes ([16f7791](https://github.com/arcadia-de/hypha/commit/16f7791ae0f5c876cfa424f833e1fb8cb10292ab))

## [1.2.0](https://github.com/arcadia-de/hypha/compare/1.1.0...v1.2.0) (2026-08-28)


### Features

* ✨ embed lua libraries into the runtime, no more hotloading ([7f96a26](https://github.com/arcadia-de/hypha/commit/7f96a2608a0a998e19957489b9c17789d2b8edd0))

## [1.1.0](https://github.com/arcadia-de/hypha/compare/v1.0.0...v1.1.0) (2026-08-28)


### Features

* :sparkles:  lua based manifest discovery ([b6307d6](https://github.com/arcadia-de/hypha/commit/b6307d698dd772caa9e044b2f0f099598c237d40))
* :sparkles: add timestamps, PODs and a (WIP) dashboard ([4fb4103](https://github.com/arcadia-de/hypha/commit/4fb41033f73e4968d5ebe4a8cff2d05f89130dd1))
* ✨ add a better event bus ([4916c2f](https://github.com/arcadia-de/hypha/commit/4916c2f93cb9f36a93ccab950b4a4822dc14086e))
* ✨ add a customizable string expander ([4916c2f](https://github.com/arcadia-de/hypha/commit/4916c2f93cb9f36a93ccab950b4a4822dc14086e))
* ✨ add a dashboard ([4fb4103](https://github.com/arcadia-de/hypha/commit/4fb41033f73e4968d5ebe4a8cff2d05f89130dd1))
* ✨ add a delta log ([cf68be0](https://github.com/arcadia-de/hypha/commit/cf68be07364f25e667f9b196d8b4ad7341c48917))
* ✨ add a more comprehensive dashboard ([4fb4103](https://github.com/arcadia-de/hypha/commit/4fb41033f73e4968d5ebe4a8cff2d05f89130dd1))
* ✨ add a new jsonschema validator ([4916c2f](https://github.com/arcadia-de/hypha/commit/4916c2f93cb9f36a93ccab950b4a4822dc14086e))
* ✨ add basic implementation for lua controllers ([4916c2f](https://github.com/arcadia-de/hypha/commit/4916c2f93cb9f36a93ccab950b4a4822dc14086e))
* ✨ add digest checking to the template controller ([c13396b](https://github.com/arcadia-de/hypha/commit/c13396b62f9089f09bd0c6909b256fde5a054d82))
* ✨ add initial implementation of docs command ([fc89db4](https://github.com/arcadia-de/hypha/commit/fc89db426a798b87061fc24d86a6ccf89f64e2f9))
* ✨ add list command ([74cfcca](https://github.com/arcadia-de/hypha/commit/74cfccadd0e0eeeb9fc794262d310bb2f6e51bc5))
* ✨ add namespaces and baked in resources ([6bbc8e0](https://github.com/arcadia-de/hypha/commit/6bbc8e0316205ca39a0c945bcc0db535daa037df))
* ✨ add OrchestratorRunMode enum ([4916c2f](https://github.com/arcadia-de/hypha/commit/4916c2f93cb9f36a93ccab950b4a4822dc14086e))
* ✨ add resource namespaces, baked in resources and some code cleanup ([6bbc8e0](https://github.com/arcadia-de/hypha/commit/6bbc8e0316205ca39a0c945bcc0db535daa037df))
* ✨ add services pattern for implementing systemd api ([23395d6](https://github.com/arcadia-de/hypha/commit/23395d64e8d0bb33fbdf3c21299def01c47672ff))
* ✨ add some basic manifest bindings utils for lua ([4916c2f](https://github.com/arcadia-de/hypha/commit/4916c2f93cb9f36a93ccab950b4a4822dc14086e))
* ✨ add some more lua bindings ([4916c2f](https://github.com/arcadia-de/hypha/commit/4916c2f93cb9f36a93ccab950b4a4822dc14086e))
* ✨ add telemetry tracking for the orchestrator ([4916c2f](https://github.com/arcadia-de/hypha/commit/4916c2f93cb9f36a93ccab950b4a4822dc14086e))
* ✨ add template controller and some code cleanup ([c13396b](https://github.com/arcadia-de/hypha/commit/c13396b62f9089f09bd0c6909b256fde5a054d82))
* ✨ add tui browse command ([b57a5e3](https://github.com/arcadia-de/hypha/commit/b57a5e3985222536f66cccda249d139390508dc7))
* ✨ add version checking using semver ([b561d20](https://github.com/arcadia-de/hypha/commit/b561d20a31e7f75c1e85e807cdb706ae9fb2d3d1))
* ✨ allow raw manifests to be written in json, yaml or jsonnet ([a7a2824](https://github.com/arcadia-de/hypha/commit/a7a28245a516305b1131042e2c4308bde150b353))
* ✨ cleanup and finish implementing state store and history ([f8e8ffc](https://github.com/arcadia-de/hypha/commit/f8e8ffc57e32e00919e5ba96678d7acf1be93a64))
* ✨ first commit ([1c8f873](https://github.com/arcadia-de/hypha/commit/1c8f8738ff3e907e7f9264251900c6ac23bc0cd6))
* ✨ fix filtering using list ([6bbc8e0](https://github.com/arcadia-de/hypha/commit/6bbc8e0316205ca39a0c945bcc0db535daa037df))
* ✨ implement kahn priority scheduling ([3ee355a](https://github.com/arcadia-de/hypha/commit/3ee355a8ae36a08f7b0db8fc635aa08c8869f0ca))
* ✨ implement the describe command ([a6541fb](https://github.com/arcadia-de/hypha/commit/a6541fb627def5fb1ebb4ccfbdebb3146668dd1c))
* ✨ initial plan implementation ([4916c2f](https://github.com/arcadia-de/hypha/commit/4916c2f93cb9f36a93ccab950b4a4822dc14086e))
* ✨ lots of additions and code cleanup ([4916c2f](https://github.com/arcadia-de/hypha/commit/4916c2f93cb9f36a93ccab950b4a4822dc14086e))
* ✨ luarocks ([4916c2f](https://github.com/arcadia-de/hypha/commit/4916c2f93cb9f36a93ccab950b4a4822dc14086e))
* ✨ make a better planner model ([fa6eaa4](https://github.com/arcadia-de/hypha/commit/fa6eaa4efafcc2a0502377e9b64197f8113d4f13))
* ✨ make list command actually list resources ([74cfcca](https://github.com/arcadia-de/hypha/commit/74cfccadd0e0eeeb9fc794262d310bb2f6e51bc5))
* ✨ make manifest loading unified for json/yaml/jsonnet ([ed4384b](https://github.com/arcadia-de/hypha/commit/ed4384b04a324b9af51f059b96fe8a942a6cca92))
* ✨ make resource kinds an int64 ([3c87d49](https://github.com/arcadia-de/hypha/commit/3c87d49753cab7f0b7c110a317cc9f51f5c05cce))
* ✨ working on the template controller ([c13396b](https://github.com/arcadia-de/hypha/commit/c13396b62f9089f09bd0c6909b256fde5a054d82))
* **dashboard:** ✨ add /api/kinds endpoint ([34d1420](https://github.com/arcadia-de/hypha/commit/34d142009156ca49b4d762e7d1b59fad8344d0f9))
* **dashboard:** ✨ get kinds from service api ([6f25e68](https://github.com/arcadia-de/hypha/commit/6f25e68f11298f30fd5839dba4eaae61fb3e88ba))


### Bug Fixes

* 🐛 fix issue with freeing stack allocated bitset ([4916c2f](https://github.com/arcadia-de/hypha/commit/4916c2f93cb9f36a93ccab950b4a4822dc14086e))
* 🐛 fix state test ([7f82cbe](https://github.com/arcadia-de/hypha/commit/7f82cbe6463fd29f30973a4e92d2c1ce006fef0f)), closes [#29](https://github.com/arcadia-de/hypha/issues/29)
* 🐛 fix weird pulsing and tracy linkage in non profiling builds ([19bbaf9](https://github.com/arcadia-de/hypha/commit/19bbaf92e449e660f00a8ce6ce2183409ae89ff7))
