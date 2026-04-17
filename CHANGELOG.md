# Changelog

All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](https://semver.org/).

---

## [1.0.0] — 2026-04-17

### Added
- Interactive graphical form for composing Slurm job submission scripts
- Live script preview panel that updates as fields are edited
- Profile-based configuration system via plain-text `.conf` files
- Built-in profile for BlueBEAR (University of Birmingham HPC facility)
- Generic profile for use with arbitrary Slurm-based HPC clusters
- Input validation for all fields prior to saving
- Time format validation (DD-HH:MM:SS)
- Email notification configuration (type and address)
- CPU constraint selection populated from profile
- Quality of Service (QoS) selection populated from profile
- Account field for specifying the project account
- Browse button for selecting the output script path
- Load profile from any location via file dialog
- About dialog with licence information and project links
- ESC key shortcut to close the application
- DEB package for Debian systems
- RPM package for Fedora, Rocky Linux, and Red Hat systems
- MIT Licence
