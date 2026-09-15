# Unreal engine MMD helper plugin

This plugin is designed to assist in creating MMD animations using Level Sequences within Unreal Engine.

## VMD Animation

What it can do now:

- Import VMD as a data asset
- Generate camera motion track in sequence from VMD
- Generate and mapping morph target in animation from VMD
- Generate mroph motion track in sequence from VMD

More infomations on how to use this plugin is in the [project wiki page](https://github.com/Arisego/UeMmdHelper/wiki)

> Plugin currently build and test on UE5.8

## Music Reactive Light

Drive scene light and intensity by music data processed with [librosa](https://github.com/librosa/librosa).

1. Rrocess music file and generates CSV file
2. Import CSV file with data table
3. Drive scene light with imported data in Light Sequencer
