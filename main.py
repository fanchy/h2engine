# -*- coding: utf-8 -*-

import h2ext

def cleanup():
    print('py init.....')
def cleanup():
    print('py cleanup.....')
  

CMD_LOGIN = 1
CMD_CHAT  = 2
class Player:
    def __init__(self, sessionid):
        self.sessionid = sessionid
        self.name = ''
    def sendChat(self, data):
        return h2ext.sessionSendMsg(self.sessionid, CMD_CHAT, data)
class PlayerMgr:
    def __init__(self):
        self.allSession2Player = {}
    def allocPlayer(self, sessionid):
        player = self.allSession2Player.get(sessionid)
        if not player:
            player = Player(sessionid)
            self.allSession2Player[sessionid] = player
        return player
    def delPlayer(self, sessionid):
        return self.allSession2Player.pop(sessionid, None)
    def findPlayerByName(self, name):
        for k, v in self.allSession2Player.iteritems():
            if v.name == name:
                return v
        return None
    def foreach(self, func):
        for k, v in self.allSession2Player.iteritems():
            func(v)

objMgr = PlayerMgr()
def onSessionReq(sessionid, cmd, body):
    print('onSessionReq', sessionid, cmd, body)
    ip = h2ext.getSessionIp(sessionid)
    h2ext.sessionSendMsg(sessionid, cmd, '服务器收到消息，sessionid:%d,ip:%s,cmd:%d,data:%s'%(sessionid, ip, cmd, body))
    return
    player = objMgr.allocPlayer(sessionid)
    if cmd == CMD_LOGIN:
        name = body
        oldPlayer = objMgr.findPlayerByName(name)
        if oldPlayer:
            h2ext.sessionClose(oldPlayer.sessionid)
            objMgr.delPlayer(sessionid)
        player.name = name
        print('player.name', player.name)
        allPlayerList = []
        def getAllPlayer(eachPlayer):
            allPlayerList.append(eachPlayer.name)
        objMgr.foreach(getAllPlayer)
        def notifyOnline(eachPlayer):
            eachPlayer.sendChat('[%s] online ip=[%s]'%(name, h2ext.getSessionIp(sessionid)))
            eachPlayer.sendChat('current online list:%s'%(str(allPlayerList)))
        objMgr.foreach(notifyOnline)
    elif cmd == CMD_CHAT:
        msg = '[%s] say:%s'%(player.name, body)
        def notifyOnline(eachPlayer):
            eachPlayer.sendChat(msg)
        objMgr.foreach(notifyOnline)
def onSessionOffline(sessionid):
    print('onSessionOffline', sessionid)
    oldPlayer = objMgr.delPlayer(sessionid)
    if oldPlayer:
        allPlayerList = []
        def getAllPlayer(eachPlayer):
            allPlayerList.append(eachPlayer.name)
        objMgr.foreach(getAllPlayer)
        def notifyOnline(eachPlayer):
            eachPlayer.sendChat('%s offline！'%(eachPlayer.name))
            eachPlayer.sendChat('current online list:%s'%(str(allPlayerList)))
        objMgr.foreach(notifyOnline)

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
    dbhost = "mysql://127.0.0.1:3306/root/rootpassword/dbname"
    dbid   = h2ext.connectDB(dbhost, 'TestGroup')
    def cb(ret):
        print('cb', ret)
        return
    h2ext.asyncQuery(dbid, 'select * from user_log limit 1', cb)   #async
    ret = h2ext.query(dbid, 'select * from user_log limit 1')       #sync

print('main.py'+'*'*10)
def httpcb(retdata):
    print('httpcb', retdata)
#h2ext.asyncHttp("http://www.baidu.com", 1, httpcb)
def when_syncSharedData(cmd, data):
    print('when_syncSharedData', cmd, data)
  
#ret = h2ext.callFunc("Entity.totalNum")
#print('ret', type(ret), ret)
def testScriptCall(arg1 = 0, arg2 = 0, arg3 = 0, arg4 = 0, arg5 = 0, arg6 = 0, arg7 = 0, arg8 = 0, arg9 = 0):
    print('testScriptCall', arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
    return 1122334

