# -*- coding: utf-8 -*-

import h2ext

def init():
    print('py init.....')
def cleanup():
    print('py cleanup.....')

def onSessionReq(sessionid, cmd, body):
    print('onSessionReq', sessionid, cmd, body)
    ip = h2ext.getSessionIp(sessionid)
    h2ext.gateBroadcastMsg(cmd, '服务器收到消息，sessionid:%d,ip:%s,cmd:%d,data:%s'%(sessionid, ip, cmd, body))
    return
def onSessionOffline(sessionid):
    print('onSessionOffline', sessionid)
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
h2ext.callFunc("Cache.set", "b", "bbbb")
h2ext.callFunc("Cache.set", "m.n[10]", "mmm1")
cacheRet = h2ext.callFunc("Cache.get", "m.n[0]")
print("cacheRet", cacheRet)
multigetRet = h2ext.callFunc("Cache.multiget", ["m.n", "b"])
print("multigetRet", multigetRet)
cacheRet = h2ext.callFunc("Cache.get", "")
print("cacheRet", cacheRet, h2ext.callFunc("Cache.size", "m"), h2ext.callFunc("Cache.size", "m.n"))
