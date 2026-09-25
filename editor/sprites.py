# Zap
# This file contains code for displaying images of sprites in Pyamoto

from PyQt5 import QtCore
Qt = QtCore.Qt

import miyamoto.spritelib as SLib

ImageCache = SLib.ImageCache

class ZapSpriteImage_Cataquack(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            3.75,
            ImageCache['Cataquack'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('Cataquack', 'cataquack.png')

class ZapSpriteImage_Biddybud(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            3.75,
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('BiddybudRed', 'biddybud_red.png')
        SLib.loadIfNotInImageCache('BiddybudYellow', 'biddybud_yellow.png')
        SLib.loadIfNotInImageCache('BiddybudGreen', 'biddybud_green.png')
        SLib.loadIfNotInImageCache('BiddybudBlue', 'biddybud_blue.png')
        SLib.loadIfNotInImageCache('BiddybudPink', 'biddybud_pink.png')

    def dataChanged(self):
        self.style = (self.parent.spritedata[2] >> 4) + 1
        self.width = 32
            
        if self.style == 1:
            self.image = ImageCache['BiddybudRed']

        elif self.style == 2:
            self.image = ImageCache['BiddybudYellow']

        elif self.style == 3:
            self.image = ImageCache['BiddybudGreen']
    
        elif self.style == 4:
            self.image = ImageCache['BiddybudBlue']

        else:
            self.image = ImageCache['BiddybudPink']

        super().dataChanged()

###
class ZapSpriteImage_Flaptor(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            3.75,
            ImageCache['Flaptor'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('Flaptor', 'flaptor.png')

#class ZapSpriteImage_Flaptor(SLib.SpriteImage_StaticMultiple):
#    def __init__(self, parent):
#        super().__init__(
#            parent,
#            3.75,
#        )
#
#    #self.offset = (-4, -16)
#    self.aux.append(SLib.AuxiliaryTrackObject(parent, 0, 0, 0))

#    @staticmethod
#    def loadImages():
#        SLib.loadIfNotInImageCache('Flaptor', 'flaptor.png')

#    def dataChanged(self):
#        moveRange = self.parent.spritedata[3] >> 4
#        moveType = self.parent.spritedata[3] & 0xF

#        track = self.aux[0]

 #       #if moveRange not in (1, 2) or moveType not in (1, 2):
          #  track.setSize(0, 0)

  #      if moveRange == 0 or moveType == 0:
   #         track.setSize(0,0)
        
    #    else:
     #       width = round(self.width * 3.75)
      #      height = round(self.height * 3.75)

       #     if moveRange == 1:
        #        track.moveType = SLib.AuxiliaryTrackObject.Horizontal
         #       track.setSize(9 * 16, 16)

          #      track.setPos(-0.625 * 60 + width / 2, -0.625 * 60 + height / 2)

           # else:
            #    track.moveType = SLib.AuxiliaryTrackObject.Vertical
             #   track.setSize(16, 9 * 16)

              #  track.setPos(-0.625 * 60 + width / 2, -9.125 * 60 + height / 2)

       # super().dataChanged()

###

class ZapSpriteImage_FlyBones(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            3.75,
            ImageCache['FlyBones'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('FlyBones', 'fly_bones.png')

class ZapSpriteImage_Stingby(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            3.75,
            ImageCache['Stingby'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('Stingby', 'stingby.png')

class ZapSpriteImage_TimeClock(SLib.SpriteImage_Static):
    def __init__(self, parent):
            super().__init__(
                parent,
                3.75,
            )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('TimeClock', 'time_clock.png')
        SLib.loadIfNotInImageCache('TimeClock_Small', 'time_clock_small.png')
        SLib.loadIfNotInImageCache('TimeClock_Evil', 'time_clock_evil.png')
        SLib.loadIfNotInImageCache('TimeClock_EvilSmall', 'time_clock_evilsmall.png')
    
    def dataChanged(self):
        super().dataChanged()

        self.small = self.parent.spritedata[6] & 0xF
        self.evil = self.parent.spritedata[6] >> 4

        if self.small:
            self.width = 16
            self.height = 16
            self.offset = (8, 8)
        else:
            self.width = 32
            self.height = 32
            self.offset = (0, 0)
            
        if self.small and not self.evil:
            self.image = ImageCache['TimeClock_Small']
        elif self.small and self.evil:
            self.image = ImageCache['TimeClock_EvilSmall']
        elif not self.small and self.evil:
            self.image = ImageCache['TimeClock_Evil']
        else:
            self.image = ImageCache['TimeClock']

class ZapSpriteImage_AngryGrrrol(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            3.75,
            ImageCache['AngryGrrrol'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('AngryGrrrol', 'angry_grrrol.png')

class ZapSpriteImage_DonutBlock(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            3.75
        )
    
    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('DonutBlock', 'donut.png')
        SLib.loadIfNotInImageCache('DonutL', 'donutL.png')
        SLib.loadIfNotInImageCache('DonutM', 'donutM.png')
        SLib.loadIfNotInImageCache('DonutR', 'donutR.png')

    def dataChanged(self):
        super().dataChanged()

        self.widthTiles = (self.parent.spritedata[2] >> 4) + 1
        self.width = self.widthTiles * 16

        self.offset = (
            (self.widthTiles * -8) + 8,
            0,
        )

    def paint(self, painter):
        super().paint(painter)
        tileSize = 60
        totalWidth = self.widthTiles * tileSize

        if self.widthTiles == 1:
            painter.drawPixmap(0, 0, ImageCache['DonutBlock'])
        elif self.widthTiles == 2:
            painter.drawPixmap(0, 0, ImageCache['DonutL'])
            painter.drawPixmap(tileSize, 0, ImageCache['DonutR'])
        else:
            painter.drawPixmap(0, 0, ImageCache['DonutL'])
            painter.drawTiledPixmap(tileSize, 0, totalWidth - tileSize * 2, tileSize, ImageCache['DonutM'])
            painter.drawPixmap(totalWidth - tileSize, 0, ImageCache['DonutR'])

class ZapSpriteImage_FrozenDonutBlock(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            3.75
        )
    
    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('FrozenDonutBlock', 'frozen_donut.png')
        SLib.loadIfNotInImageCache('FrozenDonutL', 'frozen_donutL.png')
        SLib.loadIfNotInImageCache('FrozenDonutM', 'frozen_donutM.png')
        SLib.loadIfNotInImageCache('FrozenDonutR', 'frozen_donutR.png')

    def dataChanged(self):
        super().dataChanged()

        self.widthTiles = (self.parent.spritedata[2] >> 4) + 1
        self.width = self.widthTiles * 16

        self.offset = (
            (self.widthTiles * -8) + 8,
            0,
        )

    def paint(self, painter):
        super().paint(painter)
        tileSize = 60
        totalWidth = self.widthTiles * tileSize

        if self.widthTiles == 1:
            painter.drawPixmap(0, 0, ImageCache['FrozenDonutBlock'])
        elif self.widthTiles == 2:
            painter.drawPixmap(0, 0, ImageCache['FrozenDonutL'])
            painter.drawPixmap(tileSize, 0, ImageCache['FrozenDonutR'])
        else:
            painter.drawPixmap(0, 0, ImageCache['FrozenDonutL'])
            painter.drawTiledPixmap(tileSize, 0, totalWidth - tileSize * 2, tileSize, ImageCache['FrozenDonutM'])
            painter.drawPixmap(totalWidth - tileSize, 0, ImageCache['FrozenDonutR'])

class ZapSpriteImage_StringBank(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            3.75,
            ImageCache['StringBank'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('StringBank', 'string_bank.png')

class ZapSpriteImage_ActorSpawner(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            3.75,
            ImageCache['ActorSpawner'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('ActorSpawner', 'spawn.png')

class ZapSpriteImage_NybbleBank(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            3.75,
            ImageCache['NybbleBank'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('NybbleBank', 'nybble_bank.png')

class ZapSpriteImage_Clef(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            3.75,
            ImageCache['Clef'],
            (0, -14),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('Clef', 'clef.png')

class ZapSpriteImage_Note(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            3.75,
            ImageCache['Note'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('Note', 'musicnote.png')

class ZapSpriteImage_MagicPlatform(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            3.75,
            ImageCache['MagicPlatform'],
            (-8, -8),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('MagicPlatform', 'magic_platform.png')

ImageClasses = {
    "zap:cataquack": ZapSpriteImage_Cataquack,
    "zap:para_biddybud": ZapSpriteImage_Biddybud,
    "zap:flaptor": ZapSpriteImage_Flaptor,
    "zap:stingby": ZapSpriteImage_Stingby,
    "zap:flybones": ZapSpriteImage_FlyBones,
    "zap:timeclock": ZapSpriteImage_TimeClock,
    "zap:angrygrrrol": ZapSpriteImage_AngryGrrrol,
    "zap:donut_block": ZapSpriteImage_DonutBlock,
    "zap:frozen_donut_block": ZapSpriteImage_FrozenDonutBlock,
    "zap:string_bank": ZapSpriteImage_StringBank,
    "zap:actor_spawner_simple": ZapSpriteImage_ActorSpawner,
    "zap:actor_spawner_ex": ZapSpriteImage_ActorSpawner,
    "zap:nybble_bank": ZapSpriteImage_NybbleBank,
    "zap:clef": ZapSpriteImage_Clef,
    "zap:note": ZapSpriteImage_Note,
    "zap:magicplatform": ZapSpriteImage_MagicPlatform,
}
