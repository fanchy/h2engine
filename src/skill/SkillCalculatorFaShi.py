# -*- coding: utf-8 -*-
import random
from base   import Base
from mapmgr import MapMgr
import msgtype.ttypes as MsgDef
from model.skill import SkillCalculatorBase
import ffext
import random
import weakref

calRealMagicHurt = SkillCalculatorBase.calRealMagicHurt
#对目标造成魔法攻击105%的魔法伤害
class PuGong(Base.BaseObj):
    def __init__(self, a = 105):
        self.paramHurt = a
    def exe(self, owner, skill, objDest, param):
        hurtResult = calRealMagicHurt(owner, objDest, skill, self)
        objDest.subHurtResult(owner, hurtResult)
        return
#220	引动天雷-1	对目标敌人造成125%的魔法伤害，有30%几率对目标周围造成15%魔法伤害
class YinDongTianLei(PuGong):
    def __init__(self, a = 125, b = 30, c = 15, d=3):
        PuGong.__init__(self, a)
        self.paramNomal= a
        self.paramRate = b
        #以下这个参数实际是 溅射伤害
        self.paramJiansheAttack = c
        self.paramAttackRange = d
    def exe(self, owner, skill, objDest, param):
        self.paramHurt = self.paramNomal
        PuGong.exe(self, owner, skill, objDest, param)
        if random.randint(1, 100) <= self.paramRate:
            self.paramHurt = self.paramJiansheAttack
            def cb(objDestNew):
                if objDestNew != owner and objDestNew != objDest:
                    PuGong.exe(self, owner, skill, objDestNew, param)
            owner.mapObj.foreachObjInRange(objDest.x, objDest.y, self.paramAttackRange, cb) #todo 距离
        return



#230	冰封之术-1	对目标造成魔法攻击100%的魔法伤害，同时5秒内减速目标15%移动速度
class BingFengZhiShu(PuGong):
    def __init__(self, a = 100, b = 15, c=5):
        PuGong.__init__(self, a)
        self.paramNomal= a
        self.paramJiansu = b
        self.paramSec = c
    def exe(self, owner, skill, objDest, param):
        self.paramHurt = self.paramNomal
        PuGong.exe(self, owner, skill, objDest, param)
        buff = objDest.buffCtrl.getBuff(MsgDef.BuffType.JIAN_SU)
        if buff:
            return
        objDest.buffCtrl.addBuff(MsgDef.BuffType.JIAN_SU, self.paramSec, self.paramJiansu, 230)
        objDest.updateBuffMsg()
        return




#240	神明护体-1	魔法的能量环绕身体,吸收自身魔法攻击的25%的伤害，持续240s
class ShenMingFuTi:
    def __init__(self, a = 120, b = 25):
        self.paramSec  = a
        self.paramMianShang = b
    def exe(self, owner, skill, objDest, param):
        buff = owner.buffCtrl.getBuff(MsgDef.BuffType.SHEN_MING_HU_TI)
        if buff:
            return
        owner.buffCtrl.addBuff(MsgDef.BuffType.SHEN_MING_HU_TI, self.paramSec, self.paramMianShang, 240)
        owner.hurtAbsorbLimit = int(random.randint(owner.magicAttackMin, owner.magicAttackMax) * (float(self.paramMianShang) / 100))
        owner.updateBuffMsg()
        return
#250	陨石暴落-1	一颗陨石从天而降，对单个目标造成250%的魔法伤害，5秒内造成2次附带灼烧25%魔法伤害
class YunShiBaoLuo(PuGong):
    def __init__(self, a = 250, b = 5, c = 2, d = 25):
        PuGong.__init__(self, a)
        self.paramNomal= a
        self.paramSec = b
        self.times    = c
        self.paramHurtRate= d
    def exe(self, owner, skill, objDest, param):
        self.paramHurt = self.paramNomal
        PuGong.exe(self, owner, skill, objDest, param)
        objDest.tmpInfo['_zhuoshao_times'] = self.times
        def cb():
            if not objDest:
                return
            if not objDest.tmpInfo['_zhuoshao_times']:
                return
            else:
                objDest.tmpInfo['_zhuoshao_times'] -= 1
            self.paramHurt = self.paramHurtRate
            PuGong.exe(self, owner, skill, objDest, param)
            objDest.showEffect(MsgDef.EffectType.EFFECT_ZHUO_SHAO)
            ffext.timer(int(self.paramSec/self.times)*1000, cb)
        ffext.timer(int(self.paramSec/self.times)*1000, cb)
        return




#260	天火燎原-1	对目标造成120%的魔法伤害，同时对目标周围范围的目标造成35%溅射伤害。
class TianHuoLiaoYuan(PuGong):
    def __init__(self, a = 120, b = 35, c = 3):
        PuGong.__init__(self, a)
        self.paramNomal= a
        self.paramHurtRate= b
        self.paramAttackRange = c
    def exe(self, owner, skill, objDest, param):
        self.paramHurt = self.paramNomal
        PuGong.exe(self, owner, skill, objDest, param)
        self.paramHurt = self.paramHurtRate
        def cb(objDestNew):
            if objDestNew != owner and objDest != objDestNew:
                PuGong.exe(self, owner, skill, objDestNew, param)

        owner.mapObj.foreachObjInRange(objDest.x, objDest.y, self.paramAttackRange, cb)  # todo 距离
        return

#1002	法师无双-龙啸九天	对目标周围敌人造成魔法攻击250%的魔法伤害，同时击退周围的敌人
class FaShiWuShuang(PuGong):
    def __init__(self, a = 250, b=10, c=10):
        PuGong.__init__(self, a)
        self.paramNomal= a
        self.distance  = b
        self.paramJiTui = c
    def exe(self, owner, skill, objDest, param):
        self.paramHurt = self.paramNomal
        #PuGong.exe(self, owner, skill, objDest, param)
        def cb(objDestNew):
            if objDestNew == owner or objDestNew.isDeath():
                return
            oldX = objDestNew.x
            oldY = objDestNew.y
            oldDirection = Base.getDirection(owner.x, owner.y, oldX, oldY)
            #PuGong.exe(self, owner, skill, objDestNew, param)
            #ffext.dump("-------------", objDestNew.name)
            for k in range(0, self.paramJiTui+1 - 2):
                newX, newY = Base.getPosByDirection(oldX, oldY, oldDirection, self.paramJiTui+1 - k)
                if objDestNew.mapObj.canMove(newX, newY):
                    objDestNew.mapObj.moveObj(objDestNew, newX, newY, False)
                    objDestNew.tmpInfo['targetNewX'] = newX
                    objDestNew.tmpInfo['targetNewY'] = newY
                    #ffext.dump("-----------------", newX, newY)
            PuGong.exe(self, owner, skill, objDestNew, param)
        if self.distance < 5:
            self.distance = 5#TODO 为了测试击退，调大距离
        owner.mapObj.foreachObjInRange(owner.x, owner.y, self.distance, cb)  # todo 距离
        return


# 法师超级装备 3002
class FaShiSuper(PuGong):
    def __init__(self, a = 250, b = 3, c = 100):
        PuGong.__init__(self, a)
        self.paramAttackLess= b
        self.attackRange = c
    def exe(self, owner, skill, objDest, param):
        def cb(objDest):
            if objDest != owner:
                PuGong.exe(self, owner, skill, objDest, param)
                buff = owner.buffCtrl.getBuff(MsgDef.BuffType.ATTACKLESS)
                if buff:
                    return
                objDest.buffCtrl.addBuff(MsgDef.BuffType.ATTACKLESS, self.paramAttackLess, 0, 3002)  # TODO 为了展示效果暂时设置3秒
                objDest.updateBuffMsg()
        owner.mapObj.foreachObjInRange(objDest.x, objDest.y, self.attackRange, cb)  # todo 距离


#张宝技能2
class ZhangBao2(PuGong):
    def __init__(self, a = 125, b = 30, c = 15, d=3):
        PuGong.__init__(self, a)
        self.paramNomal= a
        self.paramRate = b
        #以下这个参数实际是 溅射伤害
        self.paramJiansheAttack = c
        self.paramAttackRange = d
    def exe(self, owner, skill, objDest, param):
        self.paramHurt = self.paramNomal
        PuGong.exe(self, owner, skill, objDest, param)
        if random.randint(1, 100) <= self.paramRate:
            self.paramHurt = self.paramJiansheAttack
            def cb(objDestNew):
                if objDestNew != owner and objDestNew != objDest:
                    PuGong.exe(self, owner, skill, objDestNew, param)
            owner.mapObj.foreachPlayerInRange(objDest.x, objDest.y, self.paramAttackRange, cb) #todo 距离
        return






#孙权 4681
class SunQuan(PuGong):
    def __init__(self, a = 100, b = 15, c=5, d=3):
        PuGong.__init__(self, a)
        self.paramNomal= a
        self.paramJiansu = b
        self.paramSec = c
        self.paramRange = d
    def exe(self, owner, skill, objDest, param):
        self.paramHurt = self.paramNomal
        PuGong.exe(self, owner, skill, objDest, param)
        buff = objDest.buffCtrl.getBuff(MsgDef.BuffType.JIAN_SU)
        if buff:
            return
        objDest.buffCtrl.addBuff(MsgDef.BuffType.JIAN_SU, self.paramSec, self.paramJiansu, 4681)
        objDest.updateBuffMsg()
        def cb(objDestNew):
            if objDestNew != owner and objDest != objDestNew:
                PuGong.exe(self, owner, skill, objDestNew, param)
                objDestNew.buffCtrl.addBuff(MsgDef.BuffType.JIAN_SU, self.paramSec, self.paramJiansu, 4681)
                objDestNew.updateBuffMsg()
        owner.mapObj.foreachPlayerInRange(objDest.x, objDest.y, self.paramRange, cb)
        return




#曹操
class CaoCao(PuGong):
    def __init__(self, a = 250, b = 5, c = 2, d = 25, e=5):
        PuGong.__init__(self, a)
        self.paramNomal= a
        self.paramSec = b
        self.times    = c
        self.paramHurtRate= d
        self.paramRange = e
    def exe(self, owner, skill, objDest, param):
        self.paramHurt = self.paramNomal
        #PuGong.exe(self, owner, skill, objDest, param)

        #objDest = weakref.ref(objDest)
        #owner = weakref.ref(owner)
        #def cb(obj):
            #objDest = objDestRef()
            #owner   = ownerRef()
            #if not objDest or not owner:
                #return
            #self.paramHurt = self.paramHurtRate
            #PuGong.exe(self, owner, skill, objDest, param)
            #objDest.showEffect(MsgDef.EffectType.EFFECT_ZHUO_SHAO)
        #for k in range(0, self.times):
            #ffext.timer(int(self.paramSec * k * 1000/self.times), cb)

        def cb_new(objDestNew):
            self.paramHurt = self.paramNomal
            if objDestNew == owner:
                return
            PuGong.exe(self, owner, skill, objDestNew, param)
            objDestNew.tmpInfo['_zhuoshao_times_boss'] = self.times
            #objDestNewRef = weakref.ref(objDestNew)
            #ownerRef  = weakref.ref(owner)
            def cb():
                if not objDestNew.tmpInfo['_zhuoshao_times_boss']:
                    return
                else:
                    objDestNew.tmpInfo['_zhuoshao_times_boss'] -= 1
                self.paramHurt = self.paramHurtRate
                PuGong.exe(self, owner, skill, objDestNew, param)
                objDestNew.showEffect(MsgDef.EffectType.EFFECT_ZHUO_SHAO)
                ffext.timer(int(self.paramSec /self.times)*1000, cb)
            ffext.timer(int(self.paramSec /self.times)*1000, cb)
        owner.mapObj.foreachPlayerInRange(objDest.x, objDest.y, self.paramRange, cb_new)
        return