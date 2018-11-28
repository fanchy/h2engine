# -*- coding: utf-8 -*-

import h2ext
import json
import math
import time
import weakref
###############    Tank tutorial code begin           ####################################### 
TEAM_INIT_POS = {
    1: (6, 6),
    2: (6, 40),
    3: (40, 40),
    4: (40, 6),
}
MAX_NUM_EACH_MAP = 12
TIMER_MSEC = 50
gID = 0
def allocID():
    global gID
    gID += 1
    return gID
def sendTo(sessionid, cmd, data = ''):
    strData = json.dumps({'n':cmd, 'd':data})
    if cmd != 'update':
        print(strData)
    return sendRaw(sessionid, strData)
def sendRaw(sessionid, strData):
    return h2ext.sessionSendMsg(sessionid, 0, strData)
def calDistance(x, y, destx, desty):
    return math.sqrt((x-destx) * (x-destx) + (y - desty) * (y - desty));
class Bullet:
    def __init__(self, owner):
        self.owner = weakref.ref(owner)
        self.id = allocID()
        self.damage = 3.0
        self.speed  = 0.7
        self.radius = 0.25
        self.x = owner.x
        self.y = owner.y
        r = (-owner.angle + 90) * (math.pi / 180.0)
        self.targetX = self.x + math.cos(r) * owner.range;
        self.targetY = self.y + math.sin(r) * owner.range;
        #self.targetX = owner.x + 10
        #self.targetY = owner.y + 10
        self.tmLastUpdate = time.time()
        self.xStart  = self.x
        self.yStart  = self.y
    def getData(self):
        retData = {"id":self.id,"tank":self.owner().tankID,"x":self.x,"y":self.y,
            "tx":self.targetX,"ty":self.targetY,"sp":self.speed
        }
        return retData
    def update(self):
        needRemove = False
        tmNow = time.time()
        passedTick = int((tmNow - self.tmLastUpdate) * 1000 / 30)
        if passedTick < 1:
            return needRemove, None
        self.tmLastUpdate = tmNow
        #self.x = self.xStart
        #self.y = self.yStart
        
        owner = self.owner()
        for k in range(passedTick):
            xoffset = (self.targetX - self.x)
            yoffset = (self.targetY - self.y)
            l = 1.0 / math.sqrt((xoffset * xoffset) + (yoffset * yoffset))
            xoffset *= l
            yoffset *= l
            #print('bullet.update', self.x, self.y, self.targetX, self.targetY)
            self.x += xoffset * self.speed
            self.y += yoffset * self.speed
            #print('bullet.update', self.x, self.y, self.targetX, self.targetY)
            needRemove = False;
            if (calDistance(self.x, self.y, self.targetX, self.targetY) < 1):
                needRemove = True
            elif (self.x <= 0 or self.y <= 0 or self.x >= owner.mapObj.width or self.y >= owner.mapObj.height):
                needRemove = True
            if needRemove:
                return needRemove, None
            print(self.x, self.y)
            if not needRemove:
                if owner.mapObj.isBlock(self.x, self.y):
                    needRemove = True
                    return needRemove, None
                player = owner.mapObj.checkHitTank(self.x, self.y, owner.getID())
                if player and player != owner:
                    needRemove = True
                    return needRemove, player
        return needRemove, None
class Player:
    def __init__(self, sessionid):
        self.sessionid = sessionid
        self.mapObj    = None
        self.team      = None
        self.angle     = 0
        self.x         = 0
        self.y         = 0
        self.hp        = 10
        self.range     = 16.0;
        self.tankID    = 0
        self.score     = 0
        self.dead      = False
        self.killer    = 0
        self.allBullets= {}
    def getID(self):
        return self.sessionid
    def sendMsg(self, name, data):
        return sendTo(self.sessionid, name, data)
    def broadcast(self, name, data):
        if self.mapObj:
            self.mapObj.foreach(lambda player:player.sendMsg(name, data))
        #self.sendMsg(name, data)
        return
    def getData(self):
        retData = { 'id': self.tankID, 'team': self.team.teamID, 'owner': self.getID(),
            'pos': [ self.x, self.y ], "x":self.x,"y":self.y, 'angle': self.angle, 'hp': self.hp,
            'shield': 0, 'dead': self.dead,'score': 0 , 'killer':self.killer
        }
        return retData
    def getUpdataData(self):
        if self.hp > 0:
            return {"id":self.tankID,"x":self.x,"y":self.y,"a":self.angle,"hp":self.hp}
        return self.getData()
    def broadcastUpdateTank(self, option = None):
        tanks = []
        self.mapObj.foreach(lambda player:tanks.append(player.getUpdataData()))
        data = {"tanks":tanks}
        if option and 'bullets' in option and len(self.allBullets) > 0:
            bullets = []
            for k, bullet in self.allBullets.iteritems():
                bullets.append(bullet.getData())
            data['bullets'] = bullets
        if option and 'bulletsDelete' in option:
            data['bulletsDelete'] = option['bulletsDelete']
        self.broadcast('update', data)
        return
    def clearBullet(self):
        self.allBullets = {}
    def clearBulletByID(self, id):
        if self.allBullets.get(id):
            del self.allBullets[id]
        return True
    def addBullet(self):
        bullet = Bullet(self)
        self.allBullets[bullet.id] = bullet
        return bullet

class PlayerMgr:
    gObj = None
    @staticmethod
    def instance():
        return PlayerMgr.gObj
    def __init__(self):
        self.allPlayers = {}
    def addPlayer(self, player):
        self.allPlayers[player.getID()] = player
    def getPlayer(self, id):
        return self.allPlayers.get(id)
    def delPlayer(self, playerID):
        if self.allPlayers.get(playerID):
            del self.allPlayers[playerID]
            return True
        return False
    def foreach(self, func):
        for k, player in self.allPlayers.iteritems():
            func(player)
        return
PlayerMgr.gObj = PlayerMgr()

class Team:
    def __init__(self, id):
        self.teamID= id
        self.score = 0
        self.allPlayers = {}
    def addPlayer(self, player):
        self.allPlayers[player.getID()] = weakref.ref(player)
    def getPlayer(self, playerID):
        ret = self.allPlayers.get(playerID)
        if ret != None:
            return ret()
        return None
    def delPlayer(self, playerID):
        if self.allPlayers.get(playerID):
            del self.allPlayers[playerID]
            return True
        return False
    def getMemberNum(self):
        return len(self.allPlayers)
BLOCK_CFG = [
    [ 13.5, 2, 1, 4 ],
    [ 13.5, 12, 1, 2 ],
    [ 12.5, 13.5, 3, 1 ],
    [ 2, 13.5, 4, 1 ],
    [ 11.5, 15, 1, 2 ],
    [ 11.5, 23.5, 1, 5 ],

    [ 10, 26.5, 4, 1 ],
    [ 6, 26.5, 4, 1 ],

    [ 2, 34.5, 4, 1 ],
    [ 12.5, 34.5, 3, 1 ],
    [ 13.5, 36, 1, 2 ],
    [ 15, 36.5, 2, 1 ],
    [ 13.5, 46, 1, 4 ],

    [ 23.5, 36.5, 5, 1 ],
    [ 26.5, 38, 1, 4 ],
    [ 26.5, 42, 1, 4 ],

    [ 34.5, 46, 1, 4 ],
    [ 34.5, 36, 1, 2 ],
    [ 35.5, 34.5, 3, 1 ],
    [ 36.5, 33, 1, 2 ],
    [ 46, 34.5, 4, 1 ],

    [ 36.5, 24.5, 1, 5 ],
    [ 38, 21.5, 4, 1 ],
    [ 42, 21.5, 4, 1 ],

    [ 46, 13.5, 4, 1 ],
    [ 35.5, 13.5, 3, 1 ],
    [ 34.5, 12, 1, 2 ],
    [ 33, 11.5, 2, 1 ],
    [ 34.5, 2, 1, 4 ],

    [ 24.5, 11.5, 5, 1 ],
    [ 21.5, 10, 1, 4 ],
    [ 21.5, 6, 1, 4 ],

    # center
    [ 18.5, 22, 1, 6 ],
    [ 19, 18.5, 2, 1 ],
    [ 26, 18.5, 6, 1 ],
    [ 29.5, 19, 1, 2 ],
    [ 29.5, 26, 1, 6 ],
    [ 29, 29.5, 2, 1 ],
    [ 22, 29.5, 6, 1 ],
    [ 18.5, 29, 1, 2 ]
]
def stepRange(minNum, maxNum):
    ret = [minNum]
    step = 0.1
    curVal = minNum + step
    while True:
        if curVal >= maxNum:
            ret.append(maxNum)
            break
        ret.append(curVal)
        curVal += step
        continue
    return ret
class MapObj:
    def __init__(self, mapid):
        self.mapid  = mapid
        self.width  = 48
        self.height = 48
        self.allPos = []
        self.allPlayers = {}
        self.teams  = [Team(1), Team(2), Team(3), Team(4), ]
    def getPlayerNum(self):
        return len(self.allPlayers)
    def addPlayer(self, player):
        self.allPlayers[player.getID()] = weakref.ref(player)
    def getPlayer(self, playerID):
        ret = self.allPlayers.get(playerID)
        if ret != None:
            return ret()
        return None
    def delPlayer(self, playerID):
        if self.allPlayers.get(playerID):
            del self.allPlayers[playerID]
            for team in self.teams:
                team.delPlayer(playerID)
            return True
        return False
    def foreach(self, func):
        for k, playerref in self.allPlayers.iteritems():
            func(playerref())
        return True
    def getTeamByID(self, id):
        for team in self.teams:
            if team.teamID == id:
                return team
        return None
    def allocTeam(self, player):
        retTeam = None
        minNum    = -1
        for team in self.teams:
            if minNum == -1 or team.getMemberNum() < minNum:
                minNum = team.getMemberNum()
                retTeam = team
        player.team = retTeam
        initPos = TEAM_INIT_POS[retTeam.teamID]
        player.x = initPos[0]
        player.y = initPos[1]
        retTeam.addPlayer(player)
        return retTeam
    def allocTankID(self):
        allExistID = []
        for k, playerref in self.allPlayers.iteritems():
            player = playerref()
            allExistID.append(player.tankID)
        for k in range(1, 100):
            if k not in allExistID:
                return k
        return 1
    def checkHitTank(self, xItem, yItem, ignoreId, radius = 1):
        for k, playerref in self.allPlayers.iteritems():
            player = playerref()
            if player.getID() == ignoreId:
                continue
            print('checkHitTank', player.tankID, player.x, player.y)
            for x in range(int(xItem - radius), int(xItem + radius)):
                for y in range(int(yItem - player.range), int(yItem + player.range)):
                    if x >= player.x - player.range and x <= player.x + player.range and \
                        y >= player.y - player.range and y <= player.y + player.range:
                        return player
        return None
    def isBlock(self, xItem, yItem, radius = 1):
        minX = xItem - radius
        maxX = xItem + radius
        minY = yItem - radius
        maxY = yItem + radius
        #print(xItem, yItem, minX, maxX, minY, maxY, radius)
        if minX <= 0 or maxX >= self.width or minY <= 0 or maxY >= self.height:
            print('is block1')
            return True
        for k in BLOCK_CFG:
            centerX = k[0]
            centerY = k[1]
            halfW   = k[2] / 2.0
            halfH  = k[3] / 2.0
            
            for x in stepRange(max(0, minX), maxX):
                for y in stepRange(max(0, minY), maxY):
                    if x > centerX - halfW and x < centerX + halfW and y > centerY - halfH and y < centerY + halfH:
                        print('isblock2', x, y, centerX - halfW, centerX + halfW, centerY - halfH, centerY + halfH)
                        return True
            continue
        return False
class MapObjMgr:
    gObj = None
    @staticmethod
    def instance():
        return MapObjMgr.gObj
    def __init__(self):
        self.allMaps = {}
        return
    def allocMap(self):
        retMap = None
        for k, mapObj in self.allMaps.iteritems():
            if mapObj.getPlayerNum() < MAX_NUM_EACH_MAP:
                retMap = mapObj
                break
        if not retMap:
            retMap= MapObj(allocID())
            self.allMaps[retMap.mapid] = retMap
        return retMap
MapObjMgr.gObj = MapObjMgr()


def init():
    print('py init.....')
def cleanup():
    print('py cleanup.....')


gMsgCallBack = {}
def bind(name):
    def funcWrap(func):
        global gMsgCallBack
        gMsgCallBack[name] = func
        return func
    return funcWrap
gIndexTankID = 0
@bind('register.game')
def login(player, data):
    global gIndexTankID
    gIndexTankID += 1
    # player.x = 40
    # player.y = 40
    PlayerMgr.instance().addPlayer(player)
    player.mapObj = MapObjMgr.instance().allocMap()
    player.mapObj.addPlayer(player)
    player.mapObj.allocTeam(player)
    player.tankID = player.mapObj.allocTankID()
    print('player.tankID', player.tankID)
    
    player.sendMsg('init', {'id':player.getID()})
    player.sendMsg('tank.team', player.team.teamID)
    
    player.broadcast('tank.new', player.getData())
    def notifyTankNew(playerOther):
        if playerOther != player:
            player.sendMsg('tank.new', playerOther.getData())
        return
    player.mapObj.foreach(notifyTankNew)
    player.broadcastUpdateTank()
    return
@bind('target')
def handleTarget(player, data):
    player.angle = data
    player.broadcastUpdateTank()
    return
@bind('move')
def handleMove(player, data):
    x = player.x + data[0]
    y = player.y + data[1]
    if player.mapObj.isBlock(x, y, 0.5):
        print('cannot move')
        return False
    player.x = x
    player.y = y
    player.broadcastUpdateTank()
    print('move', player.x, player.y)
    return
def timerResetTank(playerref):
    player = playerref()
    if not player:
        return
    player.hp = 10
    player.dead = False
    player.killer = 0
    player.broadcastUpdateTank()
    return
@bind('shoot')
def handleShoot(player, data):
    print('shoot', data)
    if data:
        #player.clearBullet()
        def handleBulletTimer():
            bullet = bulletref()
            oldX = bullet.x
            oldY = bullet.y
            needRemove, hitPlayer = bullet.update()
            if needRemove:
                player.broadcastUpdateTank({'bulletsDelete':{'id':bullet.id, 'x':oldX, 'y':oldY}})
                if hitPlayer:
                    hitPlayer.hp -= bullet.damage
                    if hitPlayer.hp <= 0:
                        hitPlayer.hp = 0
                        hitPlayer.dead = True
                        bullet.owner().score += 1
                        bullet.owner().team.score += 1
                        hitPlayer.killer = bullet.owner().tankID;
                        #// respawn
                        #hitPlayer.respawn();
                        def callTimer():
                            timerResetTank(weakref.ref(hitPlayer))
                            return
                        h2ext.regTimer(3000, callTimer)
                    print('delbullet hitPlayer', bullet.id, hitPlayer.tankID)
                bullet.owner().clearBulletByID(bullet.id)
                
                player.broadcastUpdateTank()
                print('delbullet', bullet.id)
            else:
                
                h2ext.regTimer(TIMER_MSEC, handleBulletTimer)
            return
        h2ext.regTimer(TIMER_MSEC, handleBulletTimer)
        
        bulletObj = player.addBullet()
        player.broadcastUpdateTank({'bullets':True})
        bulletref = weakref.ref(bulletObj)
    return
def onSessionReq(sessionid, cmd, body):
    
    data = json.loads(body)
    name = data['n']
    if name not in ['target']:
        print('onSessionReq', sessionid, cmd, body)
    data = data['d']
    func = gMsgCallBack.get(name)
    if func:
        player = PlayerMgr.instance().getPlayer(sessionid)
        if not player:
            if name == 'register.game':
                player = Player(sessionid)
            else:
                return
        func(player, data)
        #print('call %s.%s end'%(func.__module__, func.__name__))
        return
    
    #ip = h2ext.getSessionIp(sessionid)
    #h2ext.gateBroadcastMsg(cmd, '服务器收到消息，sessionid:%d,ip:%s,cmd:%d,data:%s'%(sessionid, ip, cmd, body))
    return
###############    Tank tutorial code begin           ####################################### 
def onSessionOffline(sessionid):
    print('onSessionOffline', sessionid)
    player = PlayerMgr.instance().getPlayer(sessionid)
    if not player:
        return
    PlayerMgr.instance().delPlayer(sessionid)
    mapObj = player.mapObj
    mapObj.delPlayer(player.getID())
    player.broadcast('tank.delete', {'id':player.tankID})
    player.broadcastUpdateTank()
    player.mapObj = None
    return

def sharedMemTest():
    d = h2ext.get_shared_data(0, sessionid)
    print('shared_data', sessionid, d.strData)
def onWorkerCall(cmd, body):
    print('onWorkerCall', cmd, body)
    return "ohok"

def rpc_cb(ret):
    print('rpc_cb', ret)
#h2ext.workerRPC(0, 101, "hello", rpc_cb)


def timerTest():
    def timer_cb():
        print('timer_cb')
        return
    h2ext.regTimer(1000, timer_cb)
def dbTest():
    sql = 'create table IF NOT EXISTS foo (num integer);';
    h2ext.query(sql)
    sql = "insert into foo (num) values ('100');";
    ret = h2ext.query(sql)
    print('dbTest', ret)
    ret = h2ext.query('select * from foo limit 5')       #sync
    print('dbTest', ret)
    def dbTestCb(ret = None):
        print('dbTest asyncQuery', ret)
    h2ext.asyncQuery(0, 'select * from foo limit 5', dbTestCb)   #async
    dbname = 'myDB'
    h2ext.connectDB("sqlite://./test.db", dbname)
    ret = h2ext.queryByName(dbname, 'select * from foo limit 5')
    print('dbTest queryByName', ret)
    h2ext.asyncQueryByName(dbname, 'select * from foo limit 5', dbTestCb)
#dbTest()
print('main.py'+'*'*10)
def httpcb(retdata):
    print('httpcb', retdata)
#h2ext.asyncHttp("http://www.baidu.com", 1, httpcb)
def onSyncSharedData(cmd, data):
    print('onSyncSharedData', cmd, data)
  
#ret = h2ext.callFunc("Entity.totalNum")
#print('ret', type(ret), ret)
def testScriptCall(arg1 = 0, arg2 = 0, arg3 = 0, arg4 = 0, arg5 = 0, arg6 = 0, arg7 = 0, arg8 = 0, arg9 = 0):
    print('testScriptCall', arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
    return 1122334
h2ext.callFunc("Cache.set", "b", "bbbb")
h2ext.callFunc("Cache.set", "m.n[10]", "mmm1")
cacheRet = h2ext.callFunc("Cache.get", "m.n[0]")
print("cacheRet", cacheRet)
multigetRet = h2ext.callFunc("Cache.multiget", ["m.n", "b"])
print("multigetRet", multigetRet)
cacheRet = h2ext.callFunc("Cache.get", "")
print("cacheRet", cacheRet, h2ext.callFunc("Cache.size", "m"), h2ext.callFunc("Cache.size", "m.n"))

def test(arg = None, arg2 = None):
    print('test func', arg, arg2)
    return arg

def testTask(entity):
    h2ext.callFunc("Task.addTaskCfg", {"cfgid":1, "taskLine":2, "nextTask":2, "finishConditionType":"killMonster", "finishConditionObject":"cat", "finishConditionValue":1})
    h2ext.callFunc("Task.addTaskCfg", {"cfgid":2, "taskLine":2, "nextTask":0, "finishConditionType":"killMonster", "finishConditionObject":"dog", "finishConditionValue":1})
    taskCfg = h2ext.callFunc("Task.getTaskCfg", 1)
    print("taskCfg", taskCfg)
    h2ext.callFunc("Task.genTask", entity, 1)
    h2ext.callFunc("Task.acceptTask", entity, 1)
    h2ext.callFunc("Task.triggerEvent", entity, 'killMonster', 'cat', 1)
    h2ext.callFunc("Task.endTask", entity, 1)
    allTask = h2ext.callFunc("Task.getTask", entity, 2)
    print("testTask allTask", allTask)
    return
    
def sayhi(arg = None):
    print('sayhi *************')