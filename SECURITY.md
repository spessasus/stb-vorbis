# Security policy

## Supported versions

Only the latest release of `stb-vorbis` is supported.
Please ensure that the problem still happens there before reporting it.

## Reporting a vulnerability

Please do not open a public issue for a security problem.
Use GitHub's private vulnerability reporting instead, with the "Report a vulnerability" button in the Security tab of this repository.

Include the version, the steps or the file that trigger the problem, and what happens.

This library decodes Ogg Vorbis files, so crashes, hangs or runaway memory use on a malformed file are in scope.

The decoder itself is [`stb_vorbis`](https://github.com/nothings/stb), so a bug found there should be reported upstream.
