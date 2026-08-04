# Zap
# This file contains code for displaying images of sprites in Pyamoto

from PyQt5 import QtCore
Qt = QtCore.Qt

import miyamoto.spritelib as SLib

ImageCache = SLib.ImageCache

class SpriteImage_Cataquack(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            4.0,
            ImageCache['Cataquack'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('Cataquack', 'cataquack.png')

class SpriteImage_ParaBiddybud(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            4.0,
            ImageCache['ParaBiddybudRed'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('ParaBiddybudRed', 'para_biddybud_red.png')

class SpriteImage_Flaptor(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            4.0,
            ImageCache['Flaptor'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('Flaptor', 'flaptor.png')

class SpriteImage_FlyBones(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            4.0,
            ImageCache['FlyBones'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('FlyBones', 'fly_bones.png')

class SpriteImage_Stingby(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            4.0,
            ImageCache['Stingby'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('Stingby', 'stingby.png')

class SpriteImage_TimeClock(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            4.0,
            ImageCache['TimeClock'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('TimeClock', 'time_clock.png')

class SpriteImage_AngryGrrrol(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            4.0,
            ImageCache['AngryGrrrol'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('AngryGrrrol', 'angry_grrrol.png')

class SpriteImage_DonutBlock(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            4.0
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('DonutBlock', 'donut.png')
        SLib.loadIfNotInImageCache('DonutBlock2', 'donut_2.png')
        SLib.loadIfNotInImageCache('DonutBlock3', 'donut_3.png')
    
    def dataChanged(self):
        super().dataChanged()
        
        self.widthTiles = (self.parent.spritedata[2] >> 4) + 1
        self.width = self.widthTiles * 16
    
    def paint(self, painter):
        if self.widthTiles == 1:
            painter.drawPixmap(0, 0, ImageCache['DonutBlock'])
        elif self.widthTiles == 2:
            painter.drawPixmap(0, 0, ImageCache['DonutBlock2'])
        elif self.widthTiles == 3:
            painter.drawPixmap(0, 0, ImageCache['DonutBlock3'])
        
        super().paint(painter)

class SpriteImage_FrozenDonutBlock(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            4.0
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('FrozenDonutBlock', 'frozen_donut.png')
        SLib.loadIfNotInImageCache('FrozenDonutBlock2', 'frozen_donut_2.png')
        SLib.loadIfNotInImageCache('FrozenDonutBlock3', 'frozen_donut_3.png')
    
    def dataChanged(self):
        super().dataChanged()
        
        self.widthTiles = (self.parent.spritedata[2] >> 4) + 1
        self.width = self.widthTiles * 16
    
    def paint(self, painter):
        if self.widthTiles == 1:
            painter.drawPixmap(0, 0, ImageCache['FrozenDonutBlock'])
        elif self.widthTiles == 2:
            painter.drawPixmap(0, 0, ImageCache['FrozenDonutBlock2'])
        elif self.widthTiles == 3:
            painter.drawPixmap(0, 0, ImageCache['FrozenDonutBlock3'])
        
        super().paint(painter)

class SpriteImage_StringBank(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            4.0,
            ImageCache['StringBank'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('StringBank', 'string_bank.png')

class SpriteImage_ActorSpawner(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            4.0,
            ImageCache['ActorSpawner'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('ActorSpawner', 'spawn.png')

class SpriteImage_NybbleBank(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            4.0,
            ImageCache['NybbleBank'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('NybbleBank', 'nybble_bank.png')

ImageClasses = {
    "zap:cataquack": SpriteImage_Cataquack,
    "zap:para_biddybud": SpriteImage_ParaBiddybud,
    "zap:flaptor": SpriteImage_Flaptor,
    "zap:stingby": SpriteImage_Stingby,
    "zap:flybones": SpriteImage_FlyBones,
    "zap:timeclock": SpriteImage_TimeClock,
    "zap:angrygrrrol": SpriteImage_AngryGrrrol,
    "zap:donut_block": SpriteImage_DonutBlock,
    "zap:frozen_donut_block": SpriteImage_FrozenDonutBlock,
    "zap:string_bank": SpriteImage_StringBank,
    "zap:actor_spawner_simple": SpriteImage_ActorSpawner,
    "zap:actor_spawner_ex": SpriteImage_ActorSpawner,
    "zap:nybble_bank": SpriteImage_NybbleBank,
}
