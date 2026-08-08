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

class SpriteImage_Biddybud(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            4.0
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('BiddybudRed', 'biddybud_red.png')
        SLib.loadIfNotInImageCache('BiddybudYellow', 'biddybud_yellow.png')
        SLib.loadIfNotInImageCache('BiddybudGreen', 'biddybud_green.png')
        SLib.loadIfNotInImageCache('BiddybudBlue', 'biddybud_blue.png')
        SLib.loadIfNotInImageCache('BiddybudPink', 'biddybud_pink.png')
    
    def dataChanged(self):
        super().dataChanged()

        self.style = (self.parent.spritedata[2] >> 4) + 1
        self.width = 32
            
    def paint(self, painter):
        if self.style == 1:
            painter.drawPixmap(0, 0, ImageCache['BiddybudRed'])
        elif self.style == 2:
            painter.drawPixmap(0, 0, ImageCache['BiddybudYellow'])
        elif self.style == 3:
            painter.drawPixmap(0, 0, ImageCache['BiddybudGreen'])
        elif self.style == 4:
            painter.drawPixmap(0, 0, ImageCache['BiddybudBlue'])
        elif self.style == 5:
            painter.drawPixmap(0, 0, ImageCache['BiddybudPink'])            

        super().paint(painter)

###
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

#class SpriteImage_Flaptor(SLib.SpriteImage_StaticMultiple):
#    def __init__(self, parent):
#        super().__init__(
#            parent,
#            4.0,
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
            
    def paint(self, painter):
        if self.small and not self.evil:
            painter.drawPixmap(0, 0, ImageCache['TimeClock_Small'])
        elif self.small and self.evil:
            painter.drawPixmap(0, 0, ImageCache['TimeClock_EvilSmall'])
        elif not self.small and self.evil:
            painter.drawPixmap(0, 0, ImageCache['TimeClock_Evil'])
        else:
            painter.drawPixmap(0, 0, ImageCache['TimeClock'])        

        super().paint(painter)

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
        SLib.loadIfNotInImageCache('DonutL', 'donutL.png')
        SLib.loadIfNotInImageCache('DonutM', 'donutM.png')
        SLib.loadIfNotInImageCache('DonutR', 'donutR.png')

    def dataChanged(self):
        super().dataChanged()

        self.widthTiles = (self.parent.spritedata[2] >> 4) + 1
        self.width = self.widthTiles * 16

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

class SpriteImage_FrozenDonutBlock(SLib.SpriteImage_Static):
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
            -((self.widthTiles * 16) // 2) + (0 if self.widthTiles % 2 == 1 else 0),
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

class SpriteImage_Clef(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            4.0,
            ImageCache['Clef'],
            (0, -16),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('Clef', 'clef.png')

class SpriteImage_Note(SLib.SpriteImage_Static):
    def __init__(self, parent):
        super().__init__(
            parent,
            4.0,
            ImageCache['Note'],
            (0, 0),
        )

    @staticmethod
    def loadImages():
        SLib.loadIfNotInImageCache('Note', 'musicnote.png')

ImageClasses = {
    "zap:cataquack": SpriteImage_Cataquack,
    "zap:para_biddybud": SpriteImage_Biddybud,
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
    "zap:clef": SpriteImage_Clef,
    "zap:note": SpriteImage_Note,
}
