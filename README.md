# DumbSigner

A mutilated version of Riley Testut's [AltServer for Windows](https://github.com/rileytestut/AltServer-Windows) to sign IPAs with arbitrary p12 and mobileprovision files on Windows.

It works well enough for my needs. It probably works in Wine, too.

## Usage

```bash
> DumbSigner.exe --app=app.ipa --cert=AAAAAAAAAA.p12 --password="correct horse battery staple" --profile=Profile.mobileprovision
```

The output is a file named `app.signed.ipa`.

## Compilation

1. Install Visual Studio 2019 with desktop C++ support.
2. Use [vcpkg](https://github.com/microsoft/vcpkg) to install the packages `zlib openssl dirent cxxopts` for `x86-windows-static`.
3. Clone this repository recursively.
4. Open the solution and build the `Release` configuration for `x86`.

## Notes

* 64bit is untested.
* **You may need up to 2 times the app's size of disk space and memory to run this.**

## Roadmap

See the Projects tab. For the time being:

* Better progress indication
* Prettier (and safer) logging
* A managed .NET (Core) assembly to interface with the signing parts
* [(and more...)](https://github.com/Raymonf/DumbSigner/projects/1)

I don't really expect anyone to use this other than myself, so I won't make any promises. In any case, contributions are very welcome (just send in a PR).