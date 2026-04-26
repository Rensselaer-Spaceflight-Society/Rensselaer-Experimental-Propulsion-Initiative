# Contributing to the SFS Rocket Engine Codebase:

Hello! Thank you for your interest in contributing to the Rensselaer Spaceflight Society’s Rocket Engine Project! Below is a description of how to contribute to the Rocket Engine Project. Please read this document before opening any issues or pull requests. A copy of this document can be found on our [google drive](https://docs.google.com/document/d/1KYLMteHAQgkXGodoiJ3mANW7rw9lU86h5s6VDfJ3_Ck/edit?usp=sharing). 

This project was built in Collaboration between the Rensselaer Center for Open Source (RCOS) and the Rensselaer Spaceflight Society (SFS). Most development was done by members of RCOS and SFS, but external contributions are welcome.

## Table of Contents:

- [How to Contribute](#how-to-contribute)
- [Opening Issues](#opening-issues)
- [Branch Names](#branch-names)
- [Code Style](#code-style)
- [Commit Messages](#commit-messages)
- [Pull Requests](#pull-requests) 
- [License](#license)

## How to Contribute

If you are interested in contributing to our Rocket Engine Project, you can do so in three ways:

- Filing Bug Reports
- Fixing bugs
- Implementing Features

You can find bugs to fix or features to implement by looking at the GitHub Issues Page or by contacting the current Systems and Electronics Lead. Currently this is Cooper Werner (wernec6@rpi.edu or CooperW824 on Discord)

Be sure to read our readme to understand the command and control protocol for the engine to make sure you understand our goals for the semester.

If you are an RCOS or SFS member contact the current Systems and Electronics Lead to get access to the Spaceflight Society GitHub Organization. This will give you the ability to create branches on our repository and push directly to those branches.

If you are an external contributor, please fork the repository to have a place to store your code and then make a PR into the main repository.

## Opening Issues

We are open to hearing about bugs that you find or any ideas that you may have. You can file bug reports or feature requests on our [GitHub Issues Page.](https://github.com/Rensselaer-Spaceflight-Society/Rocket-Engine/issues)

Bug Reports:
If you have found a bug, please give a brief description of the bug and some brief steps to reproduce the bug following the template in the GitHub Repository.

Feature Requests:
If you have an idea on how to make our software better or more useful please follow the feature request template in the GitHub Issues tab. Please describe the goal of the feature in depth, and give a descriptive reason why this feature is needed. **Keep in mind that not all feature requests will be added, only those that are critical to our goal of firing the engine will be added right now.**

## Branch Names
When working on your own feature or bug request, please create a branch for that specific feature or bug fix with the following naming convention:

For bug fixes: Please name your branch `fix/<name-of-fix>`
For features: Please name your branch `feat/<feature-goal>`

## Code Style

 - Header Files:
     - File extension should be .h
     - Include guards are required
     - Contains no function implementation unless the implementation is one line (I.e. Getters / Setters)
 - Source Files:
     - File extension should be .cpp
     - DO NOT use the “using” directive for namespaces
 - QT project files(.ui, .qrc, etc.):
     - Please use QTCreator to create / modify these files to prevent bugs
 - Variable, Function, and Method Names:
     - Should be written in snake_case where all variables are all lowercase and any spaces are replaced with underscores.
     - Avoid short names. Names of variables, functions, and methods should describe what they do / represent.
 - Nesting:
     - Nesting is when you indent code blocks. Avoid having more than 3 levels of nesting in any given function. This does not include any nesting from namespaces or class definitions.
     - [See this video for tips on reducing Nesting.](https://youtu.be/CFRhGnuXG-4?si=CYWJS0oS7uM_20r8) 

## Commit Messages

You should commit often and push once a day (if you have changes you made that day, no pressure to work on this every day.)

Commit Messages should follow the [conventional commits convention.](https://www.conventionalcommits.org/en/v1.0.0/)

In summary:

Your commit message should take the form:

        commit-type(area of focus): Description of Change Made

Valid Commit Types are:
- fix: for bug fixes
- feat: for new features
- chore: for chores like updating dependencies
- docs: for updating documentation
 
Areas of focus could include:
- Hardware Schematics
- Firmware
- GUI

Example:
`feat(GUI): Add graph widgets to the UI`

If your code makes a breaking change (breaks code in other parts of the app, add a “!” before the colon)

Example:
`feat(Schematics)!: Change which pins are used for solenoid valves`

This would break other firmware code so it contains the !

## Pull Requests

Ready to merge your code? Great! Open a pull request on [GitHub.](https://github.com/Rensselaer-Spaceflight-Society/Rocket-Engine/pulls) Please give a detailed description of your pull request and what the goal of the pull request is intended to do. Someone on the Spaceflight team will review the code and merge it in when ready. 

## License
The Spaceflight Society Rocket Engine software is licensed under the MIT License:

Copyright 2024, Rensselaer Spaceflight Society

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
