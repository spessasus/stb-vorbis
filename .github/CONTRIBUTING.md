## Contributing to stb-vorbis

First, I want to thank you for wanting to help develop` stb-vorbis`.
I really appreciate your contribution :-)

Here are some tips about contributing:

### Reporting a bug

Use the GitHub issue forms and please avoid using the "Blank" template unless absolutely necessary.

### Contributing a patch/enhancement

Building requires Emscripten, see [Building from source](../README.md#building-from-source).
Run `npm run build` and `npm test` before opening a pull request.

Please do not modify `wasm/stb_vorbis.c`, it is an unmodified copy of the upstream file.
Contributions updating it to a newer release are accepted.

### License notice

When submitting a pull request to `stb-vorbis`, you are giving [Spessasus](https://github.com/spessasus) the right to license
your contributions under the [Apache License 2.0](../LICENSE), and you irrevocably permit a potential
future re-licensing of `stb-vorbis`, containing your contributions, under any open source license.
