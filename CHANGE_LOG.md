# Changelog
All notable changes to this project/module will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project/module adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---
## V1.2.0 - 08.11.2023

### Added
 - Option to disable button reading
 - Added de-initialization API function

### Changed
 - Fixing gpio definition to be compatible with proper GPIO module
 - Multiple init call does not return error state, as it has no impact if already initialized
 - Replacing file formating (spaces instead of tabs)

### Todo
 - Callback registration for long/short/custom time presses or sequences (issue #2)

---
## V1.1.0 - 04.11.2022

### Changed
 - Removed Filter dependecy and making configurable
 - Removed float32_t dependecy
 - Replace version notes with changelog

---
## V1.0.0 - 30.09.2021

### Added
 - Configuration via single table
 - Debounce filtering
 - Event callbacks on pressed and released event
 - Active & idle timings measurement
---