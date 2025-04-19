# WebForge

WebForge is a little project I'm putting together that effectively acts as a
Content Management System that runs in your terminal. The first iteration (not
public for several reasons) of this project was called ArtifactCMS, and I wrote
it in Python in about a day and a half. At the time, I was putting together a
website for my dad while also keeping SEO in mind.

## History

The first iteration (not public for *many* non-trivial reasons) of this project
was called ArtifactCMS, and I wrote it in Python in about a day and a half. At
the time, I was putting together a website for my dad that would eventually be
seen by real people and get indexed on search engines. As time moved by, and I
added patches on top of patches to keep things running, the codebase became
incredibly messy. In the end, compounding with a few events in my personal life,
I decided that it was time for a **publicly-accessible** rewrite.

### Introducing WebForge, Reborn

This new project, which will be hosted canonically on my GitHub profile at
[AdrianGjerstad/webforge](https://github.com/AdrianGjerstad/webforge), is
ArtifactCMS's intended successor, written in C++. This tool will not aim to be
a port of the former codebase, but rather a "Version 2," where command line
flags, configuration files, and internals are all reworked.

## Code Style

I aim to use the
[Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
throughout this project. This is partially due to my personal preferences and
due to the fact that I will be making heavy use of the
[Abseil C++ Library](https://github.com/abseil/abseil-cpp), developed by Google,
and the [Bazel Build System](https://bazel.build/), also developed by Google.

## License

This project is licensed under the GNU General Public License, version 3. For
details, see `LICENSE`. Every source file in this repository has a license
boilerplate at the top of the file, followed by a description of what that file
is for (see also, [Code Style](#Code-Style)).

## Contributing and Maintaining

For the time being, I intend on maintaining this project and keeping it publicly
accessible. I wouldn't have started on this rabbit hole if my search for
"command line cms" had yielded results, so I think this is something that should
be available. With that being said, GitHub
[Issues](https://github.com/AdrianGjerstad/webforge/issues) are welcome and
encouraged. I will not be accepting pull requests for now, but potentially in
the future.

