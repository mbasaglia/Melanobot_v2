Contributing to Melanobot
=========================

Issue Tracker
-------------

If you find a bug, or have a suggestion for a new feature you can report it
in the [issue tracker](https://github.com/mbasaglia/Melanobot_v2/issues) or
on IRC (#Evil.Ant.Colony on QuakeNet, highlight Melanosuchus).

### Labels

Labels are used to categorize and organize issues.

Ideally every issue should have at least an issue type label and either
code organization or module/component labels.

#### Code organization

These labels represent how the issue affects parts of the physical
representation of the codebase:

* C-Libraries - External libraries and dependencies
* C-Scripts - Helper scripts, build system etc
* C-Structure - Structure of the main codebase
* C-Test - Unit tests

#### Modules and components

These labels represent logical code structures:

* Core - Main bot code and core module
* Documentation - Code documentation, wiki
* Modules - Modules, mostly to mark features that can spawn new modules
* M-IRC - IRC module
* M-POSIX - POSIX module
* M-Script - Script module
* M-Web - Web module
* M_Xonotic - Xonotic module

#### Issue type

* T-Bug - Bug, something is broken, regressions and similar
* T-Enhancement - New features and improvements to existing features
* T-Enquiry - Something that need more testing or questions
* T-Todo - Something to do that does not fit in the other categories

#### Other

* ported - Issues from the old incarnation of Melanobot

License
-------

Melanobot is released under the terms of the GPLv3+ (see COPYING), all
contributions must be under the same license to be included into Melanobot.
