# SWS Extension

[![Build](https://github.com/myrrc/sws/actions/workflows/build.yml/badge.svg)](https://github.com/myrrc/sws/actions/workflows/build.yml)

This package can be installed with ReaPack:

```

```

[![Build status](https://ci.appveyor.com/api/projects/status/6jq0uwut3mx14xp4/branch/master?svg=true)](https://ci.appveyor.com/project/reaper-oss/sws/branch/master)

The SWS extension is a collection of features that seamlessly integrate into
REAPER, the Digital Audio Workstation (DAW) software by Cockos, Inc.

For more information on the capabilities of the SWS extension or to download it,
please visit the [end-user website](https://www.sws-extension.org).
For more information on REAPER itself or to download the latest version,
go to [reaper.fm](https://www.reaper.fm).

You are welcome to contribute! Just fork the repo and send us a pull request
(also see [Submitting pull requests](https://github.com/reaper-oss/sws/wiki/Submitting-pull-requests)
in the [wiki](https://github.com/reaper-oss/sws/wiki)).
See [Building the SWS Extension](https://github.com/reaper-oss/sws/wiki/Building-the-SWS-Extension)
for build instructions.

While not strictly required by the license, if you use the code in your own
project we would like definitely like to hear from you.

The SWS extension includes the following 3rd party libraries (which are all
similarly licensed):

- [OscPkt](http://gruntthepeon.free.fr/oscpkt/)
- [libebur128](https://github.com/jiixyj/libebur128)

The SWS extension also relies on [TagLib](https://taglib.org/) and
[WDL](https://www.cockos.com/wdl).

## Creating releases

Releases are created with tags. Create a tag, fill the necessary information about a release or a pre-release,
and then push a tag into git repository. This will trigger a CI pipeline which will create a release and upload all
necessary artifacts.

For releases, installers are also built along with libraries.

Tip: if you have a commit and a tag for release, you can push them simultaneously with `
git push --atomic origin <branch name> <tag>`.
