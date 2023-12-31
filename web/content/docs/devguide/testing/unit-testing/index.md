+++
date = "2018-02-26T11:00:13+01:00"
title = "Unit Testing"
author = "Lars Bilke"
weight = 1021

[menu]
  [menu.devguide]
    parent = "testing"
+++

## Introduction

[Unit testing](http://en.wikipedia.org/wiki/Unit_testing) can be used for testing small pieces of functionality. We use [GoogleTest](https://google.github.io/googletest/primer.html) as our test framework.

## Running Tests

GoogleTest is already included in the OGS source code. Running tests is as simple as building the `tests`-target.

{{< asciinema url="https://asciinema.org/a/249006" >}}

## Writing Tests

All tests are located inside the *Tests*-directory in library-specific subfolders. To write a new test you simply create a new *.cpp*-file and use the GoogleTest macros.
