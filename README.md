# hypha

> A declarative user-environment and dotfiles configuration system

![Hypha Logo](./assets/logo.png)

Hypha, a declarative configuration system for managing a user's environment and dotfiles.
It represents configuration as Resources defined by declarative manifests, forming a
dependency graph that is reconciled through pluggable controllers.

## Building From Source

Check out the [build docs](https://github.com/arcadia-de/hypha/wiki) for how to build.

## Wiki

Check out the [wiki](https://github.com/arcadia-de/hypha/wiki) for more information.

## Contributing

See the [Contributing guide](https://github.com/arcadia-de/hypha/wiki/Contributing) in the wiki for contribution guidelines and development information.

### AI Contributions

AI-assisted and AI-generated contributions **are welcome**.
However, AI contributions are subject to additional disclosure and review requirements described in the
[AI Contributions](https://github.com/arcadia-de/hypha/wiki/Contributing#AI) section of the Contributing guide.

## AI Usage Disclaimer

This project contains some contributions made using AI.
These contributions undergo additional review before merging.

For transparency, known AI usage in this project is documented below:

- [Claude](https://claude.ai) --- Free tier, Sonnet 5 Medium
  - Refactoring the `Resource` ID field from `char*` to `uuid_t`
  - Adding telemetry tracking and reporting data to resources
  - Additional contributions can be found in [Claude's commits](https://github.com/arcadia-de/hypha/commits?author=claude)

AI usage does not imply that the resulting changes were accepted without review.
AI-generated changes remain subject to the project's normal architectural, design, correctness, testing,
and maintainability standards, in addition to the requirements described in the AI Contributions section.

## Credits

- [helm](https://helm.sh/) --- Inspiration
- [dotbot](https://github.com/anishathalye/dotbot) --- Inspiration
- [home-manager](https://github.com/nix-community/home-manager) --- Inspiration
- [ChatGPT](https://chatgpt.com/) --- Logo
- [Claude](https://claude.ai/) --- Some [contributions](https://github.com/arcadia-de/hypha/commits?author=claude)

## License

See [LICENSE](/LICENSE).
