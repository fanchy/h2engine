# -*- coding: utf-8 -*-
import random
from base import Base
import msgtype.ttypes as MsgDef
import ffext

class CalHurtFlag:
    IGNORE_PHYSIC_DEFEND = 1 #忽略物理防御
    IGNORE_MAGIC_DEFEND = 2  #忽略魔法防御
    IGNORE_MAGIC_SHIELD = 3  #忽略魔法盾
#计算物理伤害 实际物理攻击力=随机值（最小物理攻击力，最大物理攻击力）*（1+BUFF增加系数）
def calPhysicHurt(obj):
    buff = obj.buffCtrl.getBuff(MsgDef.BuffType.DEF_HURT)
    addHurt = 0
    if buff:
        addHurt += buff.param.get(1, 0)
    return random.randint(obj.physicAttackMin, obj.physicAttackMax) * (1+addHurt/100)
#实际物理防御力=随机值（最小物理防御力，最大物理防御力）*（1+BUFF增加系数）
def calPhysicDefend(obj):
    buff = obj.buffCtrl.getBuff(MsgDef.BuffType.DEFEND)
    addDefend = 0
    if buff:
        #ffext.dump('PHYSICDEFEND',addDefend )
        addDefend += buff.param.get(1, 0)
    buff2 = obj.buffCtrl.getBuff(MsgDef.BuffType.DEF_HURT)
    if buff2:
        addDefend += buff2.param.get(1, 0)
    return random.randint(obj.physicDefendMin, max(obj.physicDefendMin, obj.physicDefendMax)) * (1 + addDefend/100)
#实际魔法攻击力=随机值（最小魔法攻击力，最大魔法攻击力）*（1+BUFF增加系数）
def calMagicHurt(obj):
    buff = obj.buffCtrl.getBuff(MsgDef.BuffType.DEF_HURT)
    addHurt = 0
    if buff:
        addHurt += buff.param.get(1, 0)
    return random.randint(obj.magicAttackMin, obj.magicAttackMax) * (1+addHurt/100)
#实际魔法防御力=随机值（最小魔法防御力，最大魔法防御力）*（1+BUFF增加系数）
def calMagicDefend(obj):
    buff = obj.buffCtrl.getBuff(MsgDef.BuffType.DEFEND)
    addDefend = 0
    if buff:
        #ffext.dump('MAGICDEFEND',addDefend )
        addDefend += buff.param.get(1, 0)
    buff2 = obj.buffCtrl.getBuff(MsgDef.BuffType.DEF_HURT)
    if buff2:
        addDefend += buff2.param.get(1, 0)
    return random.randint(obj.magicDefendMin, obj.magicDefendMax) * (1 + addDefend/100)
# 战力计算
#     平均物理攻击力=（最小物理攻击力+最大物理攻击力）/ 2
#     平均魔法攻击力=（最小魔法攻击力+最大魔法攻击力）/ 2
#     平均物理防御力=（最小物理防御力+最大物理防御力）/ 2
#     平均魔法防御力=（最小魔法防御力+最大魔法防御力）/ 2
#     ---------------------------------------------------------------
#     战力
#     =生命值/4+平均物理攻击力*4+平均魔法攻击力*4+平均物理防御力*2+平均魔法防御力*2"
def  calPowerFight(obj):
    physicAttack = (obj.physicAttackMin + obj.physicAttackMax) / 2
    magicAttack  = (obj.magicAttackMin + obj.magicAttackMax) / 2
    physicDefend = (obj.physicDefendMin + obj.physicDefendMax) / 2
    magicDefend  = (obj.magicDefendMin + obj.magicDefendMax) / 2
    power = obj.hpMax / 4 + physicAttack*4 + magicAttack*4 + physicDefend*2 + magicDefend*2
    return int(power)
#暴击计算
# if（攻击方等级-防御方等级 ） >=5  then
#    等级差暴击补偿=(攻击方等级-防御方等级)*0.05
# else
#    等级差暴击补偿=0
# ------------------------------------------------------------
# 实际暴击率=
# max( 暴击+等级差暴击补偿，0）"
# 暴击伤害计算	"物理伤害=非暴击伤害*（2）
# 魔法伤害=非暴击伤害*（2）"
def calCrit(objSrc, objDest, hurtResult):

    param = 0
    level =  max(objSrc.level - objDest.level, 0)
    if level <= 5:
        param = level * 5#百分比
    critParam = objSrc.crit + param
    rand      = random.randint(1, Base.CRIT_RATE_BASE)
    buff = objSrc.buffCtrl.getBuff(MsgDef.BuffType.CRIT)
    addCrit = 0
    if buff:
        critParam += buff.param.get(1, 0)
    if critParam >= rand:
        hurtResult.crit = 1
        hurtResult.hurt*= (1+hurtResult.crit)
        return True
    return False
# 命中计算	"if（攻击方等级-防御方等级 ） <=5  then
#    等级差命中惩罚=(攻击方等级-防御方等级)*0.05
# else
#    等级差命中惩罚=0
#
# ------------------------------------------------------------
# 实际命中率=
# max(1+ 命中-闪避+等级差命中惩罚，0）"
def calHit(objSrc, objDest):#计算是否命中
    param = 0
    level =  max(objSrc.level - objDest.level, 0)
    if level <= 5:
        param = level * 5#百分比
    hitParam = Base.HIT_RATE_BASE + objSrc.hit - objDest.avoid + param
    rand     = random.randint(1, Base.HIT_RATE_BASE)
    if hitParam >= rand:
        return True
    return False
class HurtResult:
    def __init__(self, hurt = 0, hit = 1, crit = 0):
        self.hurt = hurt
        self.hit  = hit
        self.crit = crit
        self.hurtAbsorbVal = 0#伤害吸收值
        self.hurtFlag = 0

        self.skillId = 0      #伤害的技能id
        self.uid = 0 #伤害来源
        self.targetX = 0 #目标的新位置
        self.targetY = 0 #目标的新位置

#伤害吸收计算	吸收恢复生命值=实际伤害值*伤害吸收
def calHurtAbsorb(destObj, hurtResult):
    hurtAbsorbVal = int(hurtResult.hurt * destObj.hurtAbsorb / 100)
    if hurtAbsorbVal > 0:
        hurtResult.hurtAbsorbVal = hurtAbsorbVal
        hurtResult.hurt -= hurtAbsorbVal
        return True
    return False
#吸血计算	"吸血值=实际伤害值*吸血 注意：群体伤害只计算单体最高一次的伤害"
def calHpAbsorb(srcObj, hurtVal):
    hpAbsorbVal = int(hurtVal * srcObj.hpAbsorb / 100)
    if hpAbsorbVal > 0:
        srcObj.addHPMsg(hpAbsorbVal)
    return hpAbsorbVal

#直接性伤害
#物理伤害=max(（实际物理攻击力*直接性伤害技能系数-实际物理防御力）*（1+伤害增加-伤害减免），1）
#魔法伤害=max(（实际魔法攻击力*直接性伤害技能系数-实际魔法防御力）*（1+伤害增加-伤害减免），1）
def calRealPhysicHurt(srcObj, destObj, skill, skillHurtCalculator, hitFlag = True, hpAbsorbFlag = True, ignoreDefendFlag = 0, defendParam = 0):
    hurtResult =  HurtResult()
    hurtResult.skillId = skill.skillCfg.skillId
    hurtResult.uid = srcObj.uid
    hurtResult.targetX = destObj.tmpInfo.get('targetNewX', 0)
    hurtResult.targetY = destObj.tmpInfo.get('targetNewY', 0)
    if hitFlag:#如果没有命中，返回0
        if not calHit(srcObj, destObj):
            hurtResult.hit = 0
            return hurtResult
    #伤害增加
    hurtAdd = 0
    srcBuff = srcObj.buffCtrl.getBuff(MsgDef.BuffType.GUWU_SHIQI)
    if srcBuff:
        hurtAdd += srcBuff.param.get(1, 0)
    hurtSub = 0
    defendPhyic = calPhysicDefend(destObj)
    if ignoreDefendFlag & CalHurtFlag.IGNORE_PHYSIC_DEFEND:
        defendPhyic *= defendParam
    #ffext.dump("DDDDDDDDDDDDDDDDDDDDDDDD", defendPhyic)
    realHurt = (int(calPhysicHurt(srcObj) * skillHurtCalculator.paramHurt /100) - int(defendPhyic)) * (100 + hurtAdd - hurtSub) / 100

    #ffext.dump("RRRRRRRRRRRRRRRRRRRRRRR", realHurt)
    #ffext.dump('hurt trace', calPhysicHurt(srcObj), skillHurtCalculator.paramHurt, calPhysicDefend(destObj), hurtAdd - hurtSub, realHurt)
    hurtResult.hurt = max(int(realHurt), 1)
    #计算暴击
    calCrit(srcObj, destObj, hurtResult)
    #计算伤害吸收
    calHurtAbsorb(destObj, hurtResult)
    #吸血计算
    if hpAbsorbFlag:
        calHpAbsorb(srcObj, hurtResult.hurt)
    return hurtResult
def calRealMagicHurt(srcObj, destObj, skill, skillHurtCalculator, hitFlag = True, hpAbsorbFlag = True, ignoreDefaendFlag = 0):
    hurtResult =  HurtResult()
    hurtResult.skillId = skill.skillCfg.skillId
    hurtResult.uid = srcObj.uid
    hurtResult.targetX = destObj.tmpInfo.get('targetNewX', 0)
    hurtResult.targetY = destObj.tmpInfo.get('targetNewY', 0)
    if hitFlag:#如果没有命中，返回0
        if not calHit(srcObj, destObj):
            hurtResult.hit = 0
            return hurtResult
    #伤害增加
    hurtAdd = 0
    srcBuff = srcObj.buffCtrl.getBuff(MsgDef.BuffType.GUWU_SHIQI)
    if srcBuff:
        hurtAdd += srcBuff.param.get(1, 0)
    hurtSub = 0
    defendMagic = calMagicDefend(destObj)
    if ignoreDefaendFlag & CalHurtFlag.IGNORE_MAGIC_DEFEND:
        defendMagic = 0
    realHurt = (int(calMagicHurt(srcObj) * skillHurtCalculator.paramHurt /100) - defendMagic) * (100 + hurtAdd - hurtSub) / 100
    hurtResult.hurt = max(int(realHurt), 1)
    #计算暴击
    calCrit(srcObj, destObj, hurtResult)
    #计算伤害吸收
    calHurtAbsorb(destObj, hurtResult)
    #吸血计算
    if hpAbsorbFlag:
        calHpAbsorb(srcObj, hurtResult.hurt)
    return hurtResult

#持续性伤害
#物理伤害=max(（实际物理攻击力*-实际物理防御力）*（持续性伤害系数）*（1+伤害增加-伤害减免），1）
#魔法伤害=max(（实际魔法攻击力*-实际魔法防御力）*（持续性伤害系数）（1+伤害增加-伤害减免），1）
def calTimerPhysicHurt(srcObj, destObj, skill, skillHurtCalculator):
    hurtResult =  HurtResult()
    hurtResult.skillId = skill.skillCfg.skillId
    hurtResult.uid = srcObj.uid
    hurtResult.targetX = destObj.tmpInfo.get('targetNewX', 0)
    hurtResult.targetY = destObj.tmpInfo.get('targetNewY', 0)
    #伤害增加
    hurtAdd = 0
    hurtSub = 0
    paramTimer = 0
    realHurt = (calPhysicHurt(srcObj) - calPhysicDefend(destObj)) * paramTimer / 100 * (100 + hurtAdd - hurtSub) / 100
    hurtResult.hurt = max(int(realHurt), 1)
    hurtResult.hurt = max(int(realHurt), 1)
    #计算暴击
    #calCrit(srcObj, destObj, hurtResult)
    #计算伤害吸收
    calHurtAbsorb(destObj, hurtResult)
    return realHurt
def calTimerMagicHurt(srcObj, destObj, skill, skillHurtCalculator):
    hurtResult =  HurtResult()
    hurtResult.skillId = skill.skillCfg.skillId
    hurtResult.uid = srcObj.uid
    hurtResult.targetX = destObj.tmpInfo.get('targetNewX', 0)
    hurtResult.targetY = destObj.tmpInfo.get('targetNewY', 0)
    #伤害增加
    hurtAdd = 0
    hurtSub = 0
    paramTimer = 0
    realHurt = (calMagicHurt(srcObj) - calMagicDefend(destObj)) * paramTimer / 100 * (100 + hurtAdd - hurtSub) / 100
    hurtResult.hurt = max(int(realHurt), 1)
    #计算暴击
    #calCrit(srcObj, destObj, hurtResult)
    #计算伤害吸收
    calHurtAbsorb(destObj, hurtResult)
    return hurtResult
