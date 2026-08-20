# Zap

## Overview
**Zap** (*Zenith Actor Pack*) is a **New Super Mario Bros. U** mod that adds a collection of high-quality enemies, mechanics, and utility actors for modders to use and developers to learn from.

### For Modders
- Download the **`bundle`** from the [latest release](https://github.com/Zenith-Team/Zap/releases/latest) and extract it to your mod folder, merging the `content` and `code` folders into your project. The actors will now be available in-game.
    - The `rules.txt` doesn't matter as long as the `version = 8` in your own.
- Running on console: Use the [Telkin](https://github.com/Zenith-Team/Telkin) aroma plugin to load your whole mod.
    - Place the `code`/`content` folders in `sd:/wiiu/telkin/TITLEID/` where `TITLEID` is the [title ID](https://wiiubrew.org/wiki/Title_database#00050000:_Game_Application_Titles) of your game's region (without dashes).
- Running on Cemu: Load and distribute your mod as a GraphicPack by placing it in Cemu's `graphicPacks` folder and activating it in the game's settings.

> [!WARNING]
> You must use a 2.7+ version of Cemu, Downloads can be found [here](https://cemu.info/ActionBuilds.php).

> [!IMPORTANT]
> Make sure to also install the [editor patch](https://github.com/Zenith-Team/Zap/tree/main/editor) so that you can place the actors in your levels!

### For Developers
Want to make your own custom code? Check out the [RedCore Example Mod](https://github.com/Zenith-Team/RedCore-Example-Mod) to quickly get started creating your own code mod. With Telkin, multiple mods can be loaded at once just by merging folders, so everyone can maintain their own code and share them independently.

## Compiling
Install [Tachyon](https://github.com/Zenith-Team/Tachyon) (requires [Node.js](https://nodejs.org/) v24+)
```yml
npm i -g --allow-remote=root https://github.com/Zenith-Team/Tachyon/releases/latest/download/tachyon.tgz
```
Build and run the project for your region (example with `US`)
```rb
tachyon pm install
tachyon compile US
tachyon launch US
```

## Credits
- [Luminyx](https://github.com/Luminyx1)
- [halcyonv](https://github.com/halcyon-v)
- [jhmaster](https://github.com/jhmaster2000)
- [stupidestmodder](https://github.com/stupidestmodder)

## Special Thanks
- Miguel - 3D Models
- [Baron](https://github.com/BaronStijn) - Sprite Images
- Tsuru Contributors ❤️
