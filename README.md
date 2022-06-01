Xbox ADPCM Converter
========
[![Join Discord](https://img.shields.io/badge/discord-join-7289DA.svg?logo=discord&longCache=true&style=flat)](https://discord.gg/WgAg9ymHbj)

[![Support Author](https://img.shields.io/badge/support-author-blue)](https://bit.ly/3sX2oMk) [![Help Ukraine](https://img.shields.io/badge/help-ukraine-yellow)](https://bit.ly/3afhuGm)

A tool for converting WAV files from Xbox ADPCM to PCM and vice versa.

Usage
--------
```
XboxADPCM input_file [output_file]
```
It automaticly detects the converting mode.
For example, if input file is PCM format, the output file will be Xbox ADPCM format.

Changelog
--------

**Version 1.1**
* IMA ADPCM implementation rewritten
* Code refactored using C++ 17 features
* Magic numbers now replaced with enums
* Added support of wide/unicode file names
* Removed use of Windows.h types - BYTE, WORD, DWORD
* Removed dependency on Visual C++ Redistributable

**Version 1.0**
* Original release
